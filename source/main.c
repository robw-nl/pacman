#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include "common.h"
#include "map.h"
#include "render.h"
#include "engine.h"
#include "hud.h"
#include "stats.h"
#include "input.h"

#ifdef DEBUG_MODE
char current_debug_log[256] = "pacman_debug.log";
#endif

/* Global Ghost definitions to allow external linker access */
Ghost blinky = { .x = 13.0f * 24.0f, .y = 14.0f * 24.0f, .gridX = 13, .gridY = 14, .dirX = -1, .speed = 1.5f };
Ghost pinky  = { .x = 13.0f * 24.0f, .y = 17.0f * 24.0f, .gridX = 13, .gridY = 17, .dirY = -1, .speed = 1.5f };
Ghost inky   = { .x = 11.0f * 24.0f, .y = 17.0f * 24.0f, .gridX = 11, .gridY = 17, .dirY = -1, .speed = 1.5f };
Ghost clyde  = { .x = 15.0f * 24.0f, .y = 17.0f * 24.0f, .gridX = 15, .gridY = 17, .dirY = -1, .speed = 1.5f };

int main(int argc, char* argv[]) {
    (void)argc; (void)argv;
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) return 1;
    srand(time(NULL));
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) return 1;
    if (TTF_Init() == -1) return 1;

    /* PREFER LOCAL FONT, FALLBACK TO SYSTEM DIR */
    TTF_Font* font = TTF_OpenFont("pacman-art/font/NotoSans-Bold.ttf", 16);
    if (!font) {
        font = TTF_OpenFont("/usr/share/fonts/noto/NotoSans-Bold.ttf", 16);
    }
    TTF_Font* fontSmall = TTF_OpenFont("pacman-art/font/NotoSans-Bold.ttf", 14);
    if (!fontSmall) {
        fontSmall = TTF_OpenFont("/usr/share/fonts/noto/NotoSans-Bold.ttf", 14);
    }
    if (!font || !fontSmall) {
        printf("CRITICAL ERROR: Font failed to load! Ensure font exists in pacman-art/ directory. SDL_ttf Error: %s\n", TTF_GetError());
        Mix_CloseAudio(); TTF_Quit(); SDL_Quit();
        return 1;
    }

    int hudOffset = 12;
    int windowWidth = MAP_WIDTH * 24;
    int windowHeight = (MAP_HEIGHT * 24) + hudOffset;

    SDL_Window* window = SDL_CreateWindow("Pac-Man Retro", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windowWidth, windowHeight, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    // --- AUDIO INITIALIZATION ---
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        #ifdef DEBUG_MODE
        DEBUG_LOG("SDL_mixer could not initialize! Error: %s", Mix_GetError());
        #endif
    }
    Mix_AllocateChannels(8);
    Mix_Volume(-1, 32); // max = 128

    // --- ASSET LOADING ---
    SDL_Texture* texBlinky = load_texture("pacman-art/ghosts/blinky.png", renderer);
    SDL_Texture* texPinky   = load_texture("pacman-art/ghosts/pinky.png", renderer);
    SDL_Texture* texInky    = load_texture("pacman-art/ghosts/inky.png", renderer);
    SDL_Texture* texClyde   = load_texture("pacman-art/ghosts/clyde.png", renderer);
    SDL_Texture* texScared  = load_texture("pacman-art/ghosts/blue_ghost.png", renderer);
    SDL_Texture* texFlash   = load_texture("pacman-art/ghosts/white_ghost.png", renderer);
    if (!texFlash) texFlash = texScared;

    SDL_Texture* texFruits[8];
    texFruits[0] = load_texture("pacman-art/fruits/cherry.png", renderer);
    texFruits[1] = load_texture("pacman-art/fruits/strawberry.png", renderer);
    texFruits[2] = load_texture("pacman-art/fruits/peach.png", renderer);
    texFruits[3] = load_texture("pacman-art/fruits/apple.png", renderer);
    texFruits[4] = load_texture("pacman-art/fruits/melon.png", renderer);
    texFruits[5] = load_texture("pacman-art/fruits/galaxian.png", renderer);
    texFruits[6] = load_texture("pacman-art/fruits/bell.png", renderer);
    texFruits[7] = load_texture("pacman-art/fruits/key.png", renderer);

    SDL_Texture* fallbackFruit = load_texture("pacman-art/fruits/apple.png", renderer);
    for (int i = 0; i < 8; i++) {
        if (!texFruits[i]) texFruits[i] = fallbackFruit;
    }

    SDL_Texture* texRight[3], *texLeft[3], *texUp[3], *texDown[3];
    for (int i = 0; i < 3; i++) {
        char path[128];
        snprintf(path, sizeof(path), "pacman-art/pacman-right/%d.png", i+1); texRight[i] = load_texture(path, renderer);
        snprintf(path, sizeof(path), "pacman-art/pacman-left/%d.png", i+1);  texLeft[i]  = load_texture(path, renderer);
        snprintf(path, sizeof(path), "pacman-art/pacman-up/%d.png", i+1);    texUp[i]    = load_texture(path, renderer);
        snprintf(path, sizeof(path), "pacman-art/pacman-down/%d.png", i+1);  texDown[i]  = load_texture(path, renderer);
    }

    SDL_Texture* texBanner = load_texture("pacman-art/other/pacman_banner.png", renderer);

    // --- AUDIO ASSETS ---
    AudioAssets gameAudio = {0};
    gameAudio.siren = Mix_LoadWAV("pacman-art/audio/siren.wav");
    if (gameAudio.siren)
        Mix_VolumeChunk(gameAudio.siren, 16);

    gameAudio.munch = Mix_LoadWAV("pacman-art/audio/munch.wav");
    if (gameAudio.munch)
        Mix_VolumeChunk(gameAudio.munch, 28);

    gameAudio.eatGhost = Mix_LoadWAV("pacman-art/audio/eat_ghost.wav");
    if (gameAudio.eatGhost)
        Mix_VolumeChunk(gameAudio.eatGhost, 30);

    gameAudio.death = Mix_LoadWAV("pacman-art/audio/death.wav");
    if (gameAudio.death)
        Mix_VolumeChunk(gameAudio.death, 38);

    gameAudio.start = Mix_LoadWAV("pacman-art/audio/start.wav");
    gameAudio.eatFruit = Mix_LoadWAV("pacman-art/audio/eat_fruit.wav");
    gameAudio.extraLife = Mix_LoadWAV("pacman-art/audio/extra_life.wav");
    gameAudio.powerPellet = Mix_LoadWAV("pacman-art/audio/power_pellet.wav");

    // --- INITIALIZATION ---
    GameStats sessionStats;
    load_stats(&sessionStats);

    /* SYNC UI TRACKERS WITH LOADED STATS */
    update_tracker(&difficulty_hud, sessionStats.difficulty);
    update_tracker(&maze_hud, sessionStats.mazeIndex + 1);
    update_tracker(&level_hud, 1);

    // LEVELSET 1 to 255(crash animation when finished) or beyond)
    GameContext ctx = {
        .score = 0, .level = START_LEVEL, .lives = 3, .state = STATE_START,
        .difficulty = sessionStats.difficulty, .sfx = gameAudio,
        .extraLifeThreshold = 10000,
        .idleTimer = 0, .extraLifeAwarded = 0,
        .isLocked = {0, 1, 1, 1}, .houseCounters = {0, 0, 0, 0},
        .showHelp = 0, .transitionTimer = 0,
        .killScreenTimer = 0, .killScreenShown = 0,
        .isDemo = 0, .demoTimer = 0, .engineTicks = 0
    };

    GhostMode globalGhostMode = MODE_SCATTER;
    int modeTimer = 420; // Initial Level 1 Scatter is 7 seconds (420 frames)
    int waveIndex = 0;
    int transitionTimer = 0;

    uint8_t current_maze[MAP_HEIGHT][MAP_WIDTH];
    memcpy(current_maze, master_mazes[sessionStats.mazeIndex], sizeof(current_maze));

    ctx.pelletsRemaining = 0;
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            if (current_maze[y][x] == 2 || current_maze[y][x] == 3) ctx.pelletsRemaining++;
        }
    }
    ctx.totalPellets = ctx.pelletsRemaining;

    Player player = { .x = 13.0f * 24.0f, .y = 26.0f * 24.0f, .gridX = 13, .gridY = 26, .speed = 2.0f };

    SDL_Event event;
    int running = 1;

    if (ctx.sfx.start) Mix_PlayChannel(CH_START, ctx.sfx.start, 0);
    while (running) {
        Uint32 frameStart = SDL_GetTicks();

        static int lastLoggedState = STATE_START;

        if (ctx.state != lastLoggedState) {
            DEBUG_PRINT("STATE CHANGE: %d -> %d | Pellets: %d | Time: %u\n", lastLoggedState, ctx.state, ctx.pelletsRemaining, frameStart);

            if ((lastLoggedState == STATE_START || lastLoggedState == STATE_SCORES) && ctx.state == STATE_READY) {
                globalGhostMode = MODE_SCATTER;
                waveIndex = 0;
                modeTimer = (ctx.level >= 5) ? 300 : 420;
            }

            lastLoggedState = ctx.state;
        }
        while (SDL_PollEvent(&event)) {
            if (!handle_input(&event, &player, &ctx, &sessionStats, current_maze)) {
                running = 0;
            }
        }

        if (ctx.state == STATE_START && !ctx.showHelp) {
            if (++ctx.demoTimer >= 600) {
                reset_game_state(&player, &ctx, &sessionStats, current_maze);
                ctx.isDemo = 1;
                srand(SDL_GetTicks());
                Mix_Volume(-1, 0); /* Mute all channels for arcade-accurate silent demo */
            }
        }

        // --- PRE-CALCULATE VALUES FOR TRACKERS ---
        // 1. Calculate values
        int dIdx = sessionStats.difficulty - 1;
        int mIdx = sessionStats.mazeIndex;
        int displayHighScore = (ctx.score > sessionStats.highScores[dIdx][mIdx]) ? ctx.score : sessionStats.highScores[dIdx][mIdx];
        int displayHighRound = (ctx.level > sessionStats.highLevels[dIdx][mIdx]) ? ctx.level : sessionStats.highLevels[dIdx][mIdx];

        // 2. Update Trackers (This triggers the cache logic)
        update_tracker(&score_hud, ctx.score);
        update_tracker(&high_score_hud, displayHighScore);
        update_tracker(&difficulty_hud, sessionStats.difficulty);
        update_tracker(&maze_hud, sessionStats.mazeIndex + 1);
        update_tracker(&level_hud, ctx.level);
        update_tracker(&high_level_hud, displayHighRound);

        if (ctx.state == STATE_GAMEPLAY) {
            update_tracker(&score_hud, ctx.score);
            update_tracker(&high_score_hud, displayHighScore);
            ctx.idleTimer++;

            /* DYNAMIC SIREN MANAGEMENT */
            if (player.frightenedTimer > 0 || ctx.freezeTimer > 0) {
                Mix_HaltChannel(CH_SIREN);
            } else if (ctx.sfx.siren && !Mix_Playing(CH_SIREN) && !Mix_Playing(CH_START)) {
                Mix_PlayChannel(CH_SIREN, ctx.sfx.siren, -1);
            }

            /* --- ARCADE FREEZE LOGIC --- */
            /* Halts the engine for 1 second when a ghost is eaten */
            if (ctx.freezeTimer > 0) {
                ctx.freezeTimer--;
            } else {
                ctx.engineTicks++;
                update_player(&player, current_maze, &ctx);
                handle_pellets(&player, current_maze, &ctx, renderer, hudOffset);
                handle_fruit(&player, &ctx);

                /* --- ARCADE WAVE TIMING LOGIC --- */
                /* Prevents the reversal loop: only triggers when modeTimer is > 0 and hits 0 exactly */
                if (player.frightenedTimer == 0 && waveIndex < 7 && modeTimer > 0) {
                    modeTimer--;
                    if (modeTimer == 0) {
                        waveIndex++;
                        globalGhostMode = (waveIndex % 2 == 0) ? MODE_SCATTER : MODE_CHASE;

                        /* Forced Reversal: Update grid targets so ghosts don't phase through walls */
                        Ghost* gList[] = {&blinky, &pinky, &inky, &clyde};
                        for(int i = 0; i < 4; i++) {
                            if (!gList[i]->isDead) {
                                gList[i]->gridX -= gList[i]->dirX;
                                gList[i]->gridY -= gList[i]->dirY;
                                gList[i]->dirX = -gList[i]->dirX;
                                gList[i]->dirY = -gList[i]->dirY;
                            }
                        }

                        int nextFrames = -1;
                        if (ctx.level == 1) {
                            int w1[7] = {1200, 420, 1200, 300, 1200, 300, -1};
                            nextFrames = w1[waveIndex - 1];
                        } else if (ctx.level >= 2 && ctx.level <= 4) {
                            int w2[7] = {1200, 420, 61980, 1, 61980, 1, -1};
                            nextFrames = w2[waveIndex - 1];
                        } else {
                            int w5[7] = {1200, 300, 61980, 1, -1, -1, -1};
                            nextFrames = w5[waveIndex - 1];
                        }
                        modeTimer = nextFrames;
                    }
                }

                update_blinky(&blinky, &player, current_maze, &ctx, globalGhostMode);
                update_pinky(&pinky, &player, current_maze, &ctx, globalGhostMode);
                update_inky(&inky, &player, current_maze, &ctx, globalGhostMode);
                update_clyde(&clyde, &player, current_maze, &ctx, globalGhostMode);

                // --- COLLISION LOGIC ---
                Ghost* ghosts[] = {&blinky, &pinky, &inky, &clyde};
                for (int i = 0; i < 4; i++) {
                    if (!ghosts[i]->isDead && check_collision(player.x, player.y, ghosts[i]->x, ghosts[i]->y, COLLISION_RADIUS_GHOST)) {
                        if (player.frightenedTimer > 0 && !ghosts[i]->isImmune) {
                            /* Ghost Eaten Setup */
                            ctx.score += 200 * (1 << ctx.ghostsEaten);
                            ctx.lastEatenScore = 200 * (1 << ctx.ghostsEaten);
                            ctx.eatenGhostIndex = i;
                            ctx.freezeTimer = 30; /* 0.5 Second Freeze */

                            if (ctx.ghostsEaten < 3) ctx.ghostsEaten++;
                            ghosts[i]->isDead = 1;
                            if (ctx.sfx.eatGhost) Mix_PlayChannel(CH_EAT_GHOST, ctx.sfx.eatGhost, 0);
                        } else {
                            ctx.state = STATE_DEATH;
                            ctx.deathTimer = 120;
                            #ifdef DEBUG_MODE
                            DEBUG_LOG("DEATH TRIGGERED | Score: %d | Level: %d", ctx.score, ctx.level);
                            #endif
                            Mix_HaltChannel(CH_SIREN);
                            if (ctx.sfx.death) Mix_PlayChannel(CH_DEATH, ctx.sfx.death, 0);
                        }
                    }
                }
                if (player.frightenedTimer > 0) player.frightenedTimer--;
            }
        }

        if (ctx.state == STATE_DEATH) {
            ctx.deathTimer--;
            if (ctx.deathTimer <= 0) {
                if (--ctx.lives <= 0) {
                    if (ctx.isDemo) {
                        /* Arcade Loop: Demo Over -> Return to Title Screen */
                        ctx.state = STATE_START;
                        ctx.isDemo = 0;
                        ctx.demoTimer = 0;
                        Mix_Volume(-1, 32); // Restores volume if demo completes without key interrupt [cite: 500]
                    } else {
                        /* Standard Player Game Over Logic */
                        int diffIdx = sessionStats.difficulty - 1;
                        int mazeIdx = sessionStats.mazeIndex;

                        if (ctx.level > sessionStats.highLevels[diffIdx][mazeIdx]) {
                            sessionStats.highLevels[diffIdx][mazeIdx] = ctx.level;
                        }

                        if (ctx.score > sessionStats.highScores[diffIdx][mazeIdx]) {
                            ctx.state = STATE_NAME_ENTRY;
                            ctx.nameCursorPos = 0;

                            /* Pre-fill with existing initials if present, otherwise default */
                            if (strcmp(sessionStats.initials[diffIdx][mazeIdx], "---") != 0 && sessionStats.initials[diffIdx][mazeIdx][0] != '\0') {
                                strcpy(ctx.currentName, sessionStats.initials[diffIdx][mazeIdx]);
                            } else {
                                strcpy(ctx.currentName, "A  ");
                            }
                        } else {
                            ctx.state = STATE_START;
                        }
                    }
                } else {
                    reset_entities(&player);
                    /* ARCADE POST-DEATH LOGIC: Pinky, Inky, Clyde locked. Global Counter Active. */
                    /* main.c - STATE_DEATH block */
                    ctx.isLocked[0] = 0;
                    ctx.isLocked[1] = 1; ctx.isLocked[2] = 1; ctx.isLocked[3] = 1;
                    ctx.useGlobalCounter = 1;
                    ctx.globalPelletCounter = 0;

                    memset(ctx.houseCounters, 0, sizeof(ctx.houseCounters));
                    ctx.fruitActive = 0; // Active fruit removed on death

                    ctx.ghostsEaten = 0;
                    player.frightenedTimer = 0;

                    /* FIX: Dying resets the wave timer to Scatter 1 */
                    globalGhostMode = MODE_SCATTER;
                    waveIndex = 0;
                    modeTimer = (ctx.level >= 5) ? 300 : 420;

                    ctx.state = STATE_READY;
                    transitionTimer = 90;
                }
            }
        }

        if (ctx.state == STATE_READY) {
            if (transitionTimer == 0) transitionTimer = 90;
            if (--transitionTimer <= 0) {
                /* NEW LEVEL LOGIC: Only trigger if the board was actually cleared */
                if (ctx.pelletsRemaining <= 0) {
                    ctx.level++;
                    ctx.fruitActive = 0;
                    player.frightenedTimer = 0;
                    ctx.ghostsEaten = 0;

                    /* MILESTONE LIVES: Reward survival at critical engine spikes */
                    if (ctx.level == 10 || ctx.level == 19 || (ctx.level > 19 && ctx.level % 50 == 0)) {
                        ctx.lives++;
                        if (ctx.sfx.extraLife) Mix_PlayChannel(CH_EXTRA_LIFE, ctx.sfx.extraLife, 0);
                    }

                    /* Reset the maze and pellet count only for level transitions */
                    memcpy(current_maze, master_mazes[sessionStats.mazeIndex], sizeof(current_maze));
                    ctx.pelletsRemaining = 0;
                    for (int y = 0; y < MAP_HEIGHT; y++) {
                        for (int x = 0; x < MAP_WIDTH; x++) {
                            if (current_maze[y][x] == 2 || current_maze[y][x] == 3) ctx.pelletsRemaining++;
                        }
                    }
                    ctx.totalPellets = ctx.pelletsRemaining;

                    /* Reset positions for the new level start */
                    reset_entities(&player);
                    /* NEW LEVEL LOGIC: Pinky, Inky, Clyde locked. Global Counter Off.
                     */
                    /* main.c - STATE_READY block */
                    ctx.isLocked[0] = 0;
                    ctx.isLocked[1] = 1; ctx.isLocked[2] = 1; ctx.isLocked[3] = 1;
                    ctx.useGlobalCounter = 0;
                    memset(ctx.houseCounters, 0, sizeof(ctx.houseCounters));

                    /* --- ARCADE WAVE TIMING INITIALIZATION --- */
                    globalGhostMode = MODE_SCATTER;
                    waveIndex = 0;
                    modeTimer = (ctx.level >= 5) ? 300 : 420;

                    /* EVALUATE INTERMISSION TRIGGERS */
                    if (ctx.level == 3) ctx.intermissionAct = 1;
                    else if (ctx.level == 6) ctx.intermissionAct = 2;
                    else if (ctx.level == 10 || ctx.level == 14 || ctx.level == 18) ctx.intermissionAct = 3;
                    else ctx.intermissionAct = 0;

                    if (ctx.intermissionAct > 0) {
                        ctx.state = STATE_INTERMISSION;
                        ctx.intermissionTimer = 0;
                        ctx.interX = MAP_WIDTH * 24.0f; /* Start off-screen Right */
                    } else if (ctx.level == 256 && !ctx.killScreenShown) {
                        ctx.state = STATE_KILL_SCREEN;
                        ctx.killScreenTimer = 300;
                    } else {
                        ctx.state = STATE_GAMEPLAY;
                    }
                } else {
                    /* Resume gameplay for death-respawn */
                    ctx.state = STATE_GAMEPLAY;
                }
            }
        } else if (ctx.state == STATE_INTERMISSION) {
            handle_intermission(&ctx);
        } else if (ctx.state == STATE_KILL_SCREEN) {
            ctx.killScreenTimer--;
            if (ctx.killScreenTimer <= 0) {
                ctx.state = STATE_GAMEPLAY;
                ctx.killScreenShown = 1;
            }
        }

        /* RESTORE THE SCREEN WIPE TO FIX THE TRAILS */
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        if (ctx.state == STATE_INTERMISSION) {
            render_intermission(renderer, &ctx, texLeft, texRight, texBlinky, texScared, hudOffset);
        } else if (ctx.state == STATE_KILL_SCREEN) {
            render_kill_screen_glitch(renderer, texFruits, texBlinky, hudOffset);
        } else {
            int isFlashing = 0;
            int levelClearing = (ctx.state == STATE_READY && ctx.pelletsRemaining <= 0);

            if (levelClearing && (transitionTimer / 11) % 2 == 0) {
                isFlashing = 1;
            }

            render_maze(renderer, current_maze, hudOffset, isFlashing);

            /* Hide ghosts during the Level Clear flash */
            if (!levelClearing && (ctx.state != STATE_DEATH || ctx.deathTimer > 60)) {
                Ghost* gs[] = {&blinky, &pinky, &inky, &clyde};
                SDL_Texture* txs[] = {texBlinky, texPinky, texInky, texClyde};
                for(int i=0; i<4; i++) {
                    if (gs[i]->isDead) {
                        /* Do not render eyes during the 1-second freeze */
                        if (ctx.freezeTimer == 0 || ctx.eatenGhostIndex != i) {
                            /* Authentic 1980s Floating Eyes with Directional Pupils */
                            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                            SDL_Rect e1 = {(int)gs[i]->x + 4, (int)gs[i]->y + hudOffset + 6, 6, 6};
                            SDL_Rect e2 = {(int)gs[i]->x + 14, (int)gs[i]->y + hudOffset + 6, 6, 6};
                            SDL_RenderFillRect(renderer, &e1); SDL_RenderFillRect(renderer, &e2);

                            /* Reduced 3x3 blue pupils with a minimal 1-pixel directional offset */
                            SDL_SetRenderDrawColor(renderer, 33, 33, 255, 255);
                            int pxOff = gs[i]->dirX; int pyOff = gs[i]->dirY;
                            SDL_Rect p1 = {e1.x + 1 + pxOff, e1.y + 1 + pyOff, 3, 3};
                            SDL_Rect p2 = {e2.x + 1 + pxOff, e2.y + 1 + pyOff, 3, 3};
                            SDL_RenderFillRect(renderer, &p1); SDL_RenderFillRect(renderer, &p2);
                        }
                    } else {
                        SDL_Texture* currentTarget = txs[i];

                        /* ARCADE ACCURACY: Ghosts turn blue instantly, even inside the house.
                         * Eaten ghosts returning bypass this via isImmune.
                         */
                        if (player.frightenedTimer > 0 && !gs[i]->isDead && !gs[i]->isImmune) {
                            static const int flashFrames[] = {0, 120, 120, 120, 120, 120, 120, 120, 120, 60, 120, 120, 60, 60, 120, 60, 60, 0, 60, 0};
                            int flashThreshold = (ctx.level >= 19) ? 0 : flashFrames[ctx.level];
                            currentTarget = (player.frightenedTimer < flashThreshold && (player.frightenedTimer / 10) % 2 == 0) ? texFlash : texScared;
                        }

                        /* Apply a tiny visual offset so overlapping ghosts are visible */
                        int offX = (i % 2 == 0) ? -2 : 2;
                        int offY = (i < 2) ? -2 : 2;
                        SDL_Rect r = {(int)gs[i]->x + offX, (int)gs[i]->y + hudOffset + offY, 24, 24};

                        SDL_RenderCopy(renderer, currentTarget, NULL, &r);
                    }
                }

                /* Render the score text exactly where the ghost died */
                if (ctx.freezeTimer > 0 && ctx.eatenGhostIndex >= 0) {
                    char scoreStr[16];
                    snprintf(scoreStr, sizeof(scoreStr), "%d", ctx.lastEatenScore);
                    SDL_Color cyan = {0, 255, 255, 255};
                    render_hud_text(renderer, fontSmall, scoreStr, (int)gs[ctx.eatenGhostIndex]->x, (int)gs[ctx.eatenGhostIndex]->y + hudOffset + 4, cyan);
                }

                /* Render the pink Fruit Score popup */
                if (ctx.fruitActive == 2) {
                    char fruitScoreStr[16];
                    snprintf(fruitScoreStr, sizeof(fruitScoreStr), "%d", ctx.lastFruitScore);
                    SDL_Color pink = {255, 184, 255, 255};
                    render_hud_text(renderer, fontSmall, fruitScoreStr, 13 * 24 + 4, (20 * 24) + hudOffset + 4, pink);
                }
            }

            if (ctx.state == STATE_DEATH && ctx.deathTimer <= 60) {
                int shrink = (ctx.deathTimer * 24) / 60;
                SDL_Rect pR = {(int)player.x + (24-shrink)/2, (int)player.y + hudOffset + (24-shrink)/2, shrink, shrink};
                /* SPINNING DEATH ANIMATION */
                SDL_Texture* spinTex[] = {texDown[0], texLeft[0], texUp[0], texRight[0]};
                int spinIdx = (ctx.deathTimer / 3) % 4;
                SDL_RenderCopy(renderer, spinTex[spinIdx], NULL, &pR);
            } else if (!levelClearing) {
                // --- FIX: Restore animation index ---
                int animIdx = (player.dirX != 0 || player.dirY != 0) ?
                (ctx.engineTicks / 6) % 4 : 0;
                if (animIdx == 3) animIdx = 1;

                /* Hide Pac-Man during the 1-second freeze */
                if (ctx.freezeTimer == 0) {
                    SDL_Texture** cur = (player.dirX == -1) ? texLeft : (player.dirY == -1) ? texUp : (player.dirY == 1) ? texDown : texRight;
                    SDL_Rect pR = {(int)player.x, (int)player.y + hudOffset, 24, 24};
                    SDL_RenderCopy(renderer, cur[animIdx], NULL, &pR);
                }
            }

            // REMOVE the old "HIGH SCORE TRACKER" calculation lines from here

            render_fruit(renderer, &ctx, texFruits, hudOffset);
            render_hud(renderer, font, fontSmall, ctx.score, displayHighScore, sessionStats.initials[dIdx][mIdx], ctx.lives, ctx.level, ctx.difficulty, sessionStats.mazeIndex, windowWidth, windowHeight, texLeft[0], texBanner, texFruits);
            render_overlay(renderer, font, fontSmall, ctx.state, ctx.wonLastGame, windowWidth, windowHeight, sessionStats.mazeIndex, ctx.difficulty, ctx.showHelp, hudOffset);

            if (ctx.state == STATE_NAME_ENTRY) {
                render_name_entry(renderer, font, fontSmall, &ctx, windowWidth, windowHeight);
            } else if (ctx.state == STATE_KILL_SCREEN) {
                render_kill_screen_glitch(renderer, texFruits, texBlinky, hudOffset);
            }
        }
        SDL_RenderPresent(renderer);

        Uint32 frameTime = SDL_GetTicks() - frameStart;
        if (16 > frameTime) {
            SDL_Delay(16 - frameTime);
        }
    }

    /* PROPER ASSET DEALLOCATION: Clean up textures and audio before shutdown */
    SDL_DestroyTexture(texBlinky); SDL_DestroyTexture(texPinky);
    SDL_DestroyTexture(texInky);   SDL_DestroyTexture(texClyde);
    SDL_DestroyTexture(texScared); SDL_DestroyTexture(texFlash);
    SDL_DestroyTexture(texBanner);

    for (int i = 0; i < 8; i++) {
        /* Only destroy if it's not pointing to the fallback fruit */
        if (texFruits[i] && texFruits[i] != fallbackFruit) SDL_DestroyTexture(texFruits[i]);
    }
    SDL_DestroyTexture(fallbackFruit);

    for (int i = 0; i < 3; i++) {
        SDL_DestroyTexture(texRight[i]);
        SDL_DestroyTexture(texLeft[i]);
        SDL_DestroyTexture(texUp[i]);    SDL_DestroyTexture(texDown[i]);
    }

    invalidate_maze_cache();
    invalidate_text_cache();

    Mix_FreeChunk(gameAudio.siren);     Mix_FreeChunk(gameAudio.death);
    Mix_FreeChunk(gameAudio.munch);     Mix_FreeChunk(gameAudio.eatGhost);
    Mix_FreeChunk(gameAudio.start);     Mix_FreeChunk(gameAudio.eatFruit);
    Mix_FreeChunk(gameAudio.extraLife); Mix_FreeChunk(gameAudio.powerPellet);

    /* FINAL ATOMIC SAVE: Single write to NVMe on exit */
    save_stats(&sessionStats);

    /* CLEANUP SDL SUBSYSTEMS */
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    if (font) TTF_CloseFont(font);
    if (fontSmall) TTF_CloseFont(fontSmall);

    Mix_CloseAudio();
    // Destroy textures while SDL/TTF are still active
    if (difficulty_hud.texture) SDL_DestroyTexture(difficulty_hud.texture);
    if (maze_hud.texture) SDL_DestroyTexture(maze_hud.texture);
    if (level_hud.texture) SDL_DestroyTexture(level_hud.texture);
    if (high_level_hud.texture) SDL_DestroyTexture(high_level_hud.texture);
    if (score_hud.texture) SDL_DestroyTexture(score_hud.texture);
    if (high_score_hud.texture) SDL_DestroyTexture(high_score_hud.texture);
    TTF_Quit();
    SDL_Quit();

    return 0;
}
