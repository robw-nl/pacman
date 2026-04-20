#include "input.h"
#include "engine.h"
#include <string.h>

static void handle_system_toggles(SDL_Event* event, GameContext* ctx, GameStats* stats, uint8_t maze[MAP_HEIGHT][MAP_WIDTH]) {
    if (event->key.keysym.mod & KMOD_CTRL) {
        /* 1. Difficulty Toggles */
        if (event->key.keysym.sym >= SDLK_1 && event->key.keysym.sym <= SDLK_3) {
            int diff = event->key.keysym.sym - SDLK_0;
            stats->difficulty = diff;
            ctx->difficulty = diff;
            update_tracker(&difficulty_hud, diff);
            save_stats(stats);
        }

        /* 2. Maze Toggle (Ctrl + M) */
        if (event->key.keysym.sym == SDLK_m) {
            if (ctx->state == STATE_START || ctx->state == STATE_SCORES) {
                stats->mazeIndex = (stats->mazeIndex + 1) % TOTAL_MAZES;
                update_tracker(&maze_hud, stats->mazeIndex + 1);
                save_stats(stats);
                invalidate_maze_cache();
                memcpy(maze, master_mazes[stats->mazeIndex], sizeof(uint8_t) * MAP_HEIGHT * MAP_WIDTH);

                ctx->pelletsRemaining = 0;
                for (int y = 0; y < MAP_HEIGHT; y++) {
                    for (int x = 0; x < MAP_WIDTH; x++) {
                        if (maze[y][x] == 2 || maze[y][x] == 3) ctx->pelletsRemaining++;
                    }
                }
                ctx->totalPellets = ctx->pelletsRemaining;
            }
        }

        /* 3. Wipe Stats (Ctrl + Delete) */
        if (event->key.keysym.sym == SDLK_DELETE) {
            memset(stats->highScores, 0, sizeof(stats->highScores));
            memset(stats->highLevels, 0, sizeof(stats->highLevels));
            for (int d = 0; d < 3; d++) {
                for (int m = 0; m < TOTAL_MAZES; m++) {
                    strcpy(stats->initials[d][m], "---");
                }
            }
            save_stats(stats);

            // /* Force immediate UI refresh */
            update_tracker(&high_score_hud, 0);
            update_tracker(&high_level_hud, 0);
        }
    }
}

void reset_game_state(Player* player, GameContext* ctx, GameStats* stats, uint8_t maze[MAP_HEIGHT][MAP_WIDTH]) {
    ctx->lives = 3;
    ctx->level = START_LEVEL;
    ctx->score = 0; ctx->extraLifeAwarded = 0;
    ctx->idleTimer = 0; ctx->fruitActive = 0; player->frightenedTimer = 0;

    /* FIX: Snap all entities back to start positions for a new game */
    reset_entities(player);

    /* FIX: Completely wipe the post-death ghost house state for a fresh game */
    ctx->useGlobalCounter = 0;
    ctx->globalPelletCounter = 0;
    ctx->isLocked[0] = 0;
    ctx->isLocked[1] = 1; ctx->isLocked[2] = 1; ctx->isLocked[3] = 1;

    memset(ctx->houseCounters, 0, sizeof(ctx->houseCounters));
    memcpy(maze, master_mazes[stats->mazeIndex], sizeof(uint8_t) * MAP_HEIGHT * MAP_WIDTH);
    ctx->pelletsRemaining = 0;

    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            if (maze[y][x] == 2 || maze[y][x] == 3) ctx->pelletsRemaining++;
        }
    }
    ctx->totalPellets = ctx->pelletsRemaining;
    ctx->state = STATE_READY;
    ctx->transitionTimer = 90;
}

int handle_input(SDL_Event* event, Player* player, GameContext* ctx, GameStats* stats, uint8_t maze[MAP_HEIGHT][MAP_WIDTH]) {
    if (event->type == SDL_QUIT) return 0;
    if (event->type == SDL_KEYDOWN) {
        if (ctx->isDemo) {
            ctx->isDemo = 0;
            ctx->demoTimer = 0;
            ctx->state = STATE_START;
            Mix_Volume(-1, 32); /* Restore standard volume */
            return 1;
        }

        if (event->key.keysym.sym == SDLK_ESCAPE) return 0;
        if (ctx->state == STATE_START && event->key.keysym.sym == SDLK_F1) {
            ctx->showHelp = !ctx->showHelp;
        }

        handle_system_toggles(event, ctx, stats, maze);

        if (ctx->state == STATE_START && event->key.keysym.sym == SDLK_RETURN) {
            reset_game_state(player, ctx, stats, maze);
        }

        if (ctx->state == STATE_SCORES && event->key.keysym.sym == SDLK_RETURN) {
            reset_game_state(player, ctx, stats, maze);
        }

        if (ctx->state == STATE_GAMEPLAY) {
            switch (event->key.keysym.sym) {
                case SDLK_UP:    player->nextDirX = 0;  player->nextDirY = -1; break;
                case SDLK_DOWN:  player->nextDirX = 0;  player->nextDirY = 1;  break;
                case SDLK_LEFT:  player->nextDirX = -1; player->nextDirY = 0;  break;
                case SDLK_RIGHT: player->nextDirX = 1;  player->nextDirY = 0;  break;
                case SDLK_p:
                    ctx->state = STATE_PAUSED;
                    Mix_Pause(-1);
                    break;
            }
        } else if (ctx->state == STATE_PAUSED) {
            if (event->key.keysym.sym == SDLK_p) {
                ctx->state = STATE_GAMEPLAY;
                Mix_Resume(-1);
            }
        }

        if (ctx->state == STATE_NAME_ENTRY) {
            SDL_Keycode key = event->key.keysym.sym;

            /* Keyboard Entry: Supports A-Z (Enforced CAPS) and 0-9 with Backspace support */
            if (key == SDLK_BACKSPACE && ctx->nameCursorPos > 0) {
                ctx->nameCursorPos--;
                ctx->currentName[ctx->nameCursorPos] = ' ';
            } else if (ctx->nameCursorPos < 3) {
                char c = 0;
                if (key >= SDLK_a && key <= SDLK_z) c = 'A' + (key - SDLK_a);
                else if (key >= SDLK_0 && key <= SDLK_9) c = '0' + (key - SDLK_0);

                if (c != 0) {
                    ctx->currentName[ctx->nameCursorPos] = c;
                    ctx->nameCursorPos++;
                }
            }

            if (key == SDLK_RETURN) {
                int dIdx = stats->difficulty - 1;
                int mIdx = stats->mazeIndex;
                stats->highScores[dIdx][mIdx] = ctx->score;
                memcpy(stats->initials[dIdx][mIdx], ctx->currentName, 3);
                stats->initials[dIdx][mIdx][3] = '\0';
                save_stats(stats);
                ctx->state = STATE_SCORES;
            }
        }
    }
    return 1;
}
