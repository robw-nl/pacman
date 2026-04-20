#include "render.h"
#include <SDL2/SDL_image.h>

static void fill_circle(SDL_Renderer* renderer, int cx, int cy, int radius) {
    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            if ((dx * dx + dy * dy) <= (radius * radius)) {
                SDL_RenderDrawPoint(renderer, cx + dx, cy + dy);
            }
        }
    }
}

static SDL_Texture* cachedMazeTex = NULL;

void invalidate_maze_cache(void) {
    if (cachedMazeTex) {
        SDL_DestroyTexture(cachedMazeTex);
        cachedMazeTex = NULL;
    }
}

#define TEXT_CACHE_SIZE 32

typedef struct {
    char text[64];
    SDL_Color color;
    TTF_Font* font_ptr;
    SDL_Texture* texture;
    int w, h;
    Uint32 last_used;
} TextCacheEntry;

static TextCacheEntry text_cache[TEXT_CACHE_SIZE];

void invalidate_text_cache(void) {
    for (int i = 0; i < TEXT_CACHE_SIZE; i++) {
        if (text_cache[i].texture) {
            SDL_DestroyTexture(text_cache[i].texture);
            text_cache[i].texture = NULL;
        }
    }
}

SDL_Texture* load_texture(const char* path, SDL_Renderer* renderer) {
    SDL_Texture* tex = IMG_LoadTexture(renderer, path);
    if (!tex) {
        DEBUG_LOG("Failed to load texture: %s", path);
    }
    return tex;
}

void render_tracker(SDL_Renderer* renderer, TTF_Font* font, HUD_Tracker* t, int x, int y, SDL_Color color, bool includeLabel) {
    if (t->dirty || !t->texture) {
        if (t->texture) SDL_DestroyTexture(t->texture);

        char buf[64];
        if (includeLabel) {
            snprintf(buf, sizeof(buf), "%s%d", t->label, t->value);
        } else {
            snprintf(buf, sizeof(buf), "%d", t->value);
        }

        SDL_Surface* surf = TTF_RenderText_Solid(font, buf, color);
        if (surf) {
            t->texture = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_FreeSurface(surf);
        }
        t->dirty = false;
    }

    if (t->texture) {
        int w, h;
        SDL_QueryTexture(t->texture, NULL, NULL, &w, &h);
        SDL_Rect dest = { x, y, w, h };
        SDL_RenderCopy(renderer, t->texture, NULL, &dest);
    }
}

void render_hud_text(SDL_Renderer* renderer, TTF_Font* font, const char* text, int x, int y, SDL_Color color) {
    if (!font || !text || text[0] == '\0') return;

    Uint32 now = SDL_GetTicks();
    int oldest_idx = 0;
    Uint32 oldest_time = 0xFFFFFFFF;

    /* 1. Cache Hit Lookup */
    for (int i = 0; i < TEXT_CACHE_SIZE; i++) {
        if (text_cache[i].texture && text_cache[i].font_ptr == font &&
            text_cache[i].color.r == color.r && text_cache[i].color.g == color.g &&
            text_cache[i].color.b == color.b && text_cache[i].color.a == color.a &&
            strcmp(text_cache[i].text, text) == 0) {

            text_cache[i].last_used = now;
        SDL_Rect dest = {x, y, text_cache[i].w, text_cache[i].h};
        SDL_RenderCopy(renderer, text_cache[i].texture, NULL, &dest);
        return;
            }
            if (text_cache[i].last_used < oldest_time) {
                oldest_time = text_cache[i].last_used;
                oldest_idx = i;
            }
    }

    /* 2. Cache Miss: Render, evict oldest, and store */
    // comment out pr remove DEBUG_PRINT
    DEBUG_PRINT("Cache Miss: Allocating texture for '%s'", text);
    SDL_Surface* surface = TTF_RenderText_Solid(font, text, color);

    if (surface) {
        if (text_cache[oldest_idx].texture) {
            SDL_DestroyTexture(text_cache[oldest_idx].texture);
        }
        text_cache[oldest_idx].texture = SDL_CreateTextureFromSurface(renderer, surface);
        text_cache[oldest_idx].w = surface->w;
        text_cache[oldest_idx].h = surface->h;
        text_cache[oldest_idx].color = color;
        text_cache[oldest_idx].font_ptr = font;
        strncpy(text_cache[oldest_idx].text, text, sizeof(text_cache[oldest_idx].text) - 1);
        text_cache[oldest_idx].text[sizeof(text_cache[oldest_idx].text) - 1] = '\0';
        text_cache[oldest_idx].last_used = now;

        SDL_Rect dest = {x, y, surface->w, surface->h};
        SDL_RenderCopy(renderer, text_cache[oldest_idx].texture, NULL, &dest);
        SDL_FreeSurface(surface);
    }
}

#define TS 24

void render_maze(SDL_Renderer* renderer, uint8_t maze[MAP_HEIGHT][MAP_WIDTH], int hudOffset, int isFlashing) {
    if (!cachedMazeTex) {
        cachedMazeTex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, MAP_WIDTH * TS, MAP_HEIGHT * TS);
        SDL_SetTextureBlendMode(cachedMazeTex, SDL_BLENDMODE_BLEND);
        SDL_SetRenderTarget(renderer, cachedMazeTex);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);

        // --- PASS 1: The Solid Corridors (Rendered White for ColorMod) ---
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        for(int y = 0; y < MAP_HEIGHT; y++) {
            for(int x = 0; x < MAP_WIDTH; x++) {
                if(maze[y][x] == 1) {
                    int cx = x * TS + 12;
                    int cy = y * TS + 12;

                    fill_circle(renderer, cx, cy, 8);

                    if(x < MAP_WIDTH-1 && maze[y][x+1] == 1) {
                        SDL_Rect r = {cx, cy - 8, 25, 16};
                        SDL_RenderFillRect(renderer, &r);
                    }
                    if(y < MAP_HEIGHT-1 && maze[y+1][x] == 1) {
                        SDL_Rect d = {cx - 8, cy, 16, 25};
                        SDL_RenderFillRect(renderer, &d);
                    }
                }
            }
        }

        // --- PASS 2: The Black Hollow Core ---
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        for(int y = 0; y < MAP_HEIGHT; y++) {
            for(int x = 0; x < MAP_WIDTH; x++) {
                if(maze[y][x] == 1) {
                    int cx = x * TS + 12;
                    int cy = y * TS + 12;

                    fill_circle(renderer, cx, cy, 4);

                    if(x < MAP_WIDTH-1 && maze[y][x+1] == 1) {
                        SDL_Rect r = {cx, cy - 4, 25, 8};
                        SDL_RenderFillRect(renderer, &r);
                    }
                    if(y < MAP_HEIGHT-1 && maze[y+1][x] == 1) {
                        SDL_Rect d = {cx - 4, cy, 8, 25};
                        SDL_RenderFillRect(renderer, &d);
                    }

                    if(x < MAP_WIDTH-1 && y < MAP_HEIGHT-1 &&
                        maze[y][x+1] == 1 && maze[y+1][x] == 1 && maze[y+1][x+1] == 1) {
                        SDL_Rect f = {cx, cy, 25, 25};
                    SDL_RenderFillRect(renderer, &f);
                }
            }
        }
    }
    SDL_SetRenderTarget(renderer, NULL);
    } /* This brace closes: if (!cachedMazeTex) */

    /* --- DRAW THE CACHED WALLS --- */
    if (isFlashing) {
        SDL_SetTextureColorMod(cachedMazeTex, 255, 255, 255);
    } else {
        SDL_SetTextureColorMod(cachedMazeTex, 24, 83, 125);
    }

    SDL_Rect dest = {0, hudOffset, MAP_WIDTH * TS, MAP_HEIGHT * TS};
    SDL_RenderCopy(renderer, cachedMazeTex, NULL, &dest);

    /* --- PASS 3: Gates, Pellets, and Energizers --- */
    for(int y = 0; y < MAP_HEIGHT; y++) {
        for(int x = 0; x < MAP_WIDTH; x++) {
            if(maze[y][x] == 4) { // Ghost Gate
                SDL_SetRenderDrawColor(renderer, 255, 182, 255, 255);
                SDL_Rect gate = {x*TS, y*TS + 10 + hudOffset, TS, 4};
                SDL_RenderFillRect(renderer, &gate);
            } else if (maze[y][x] == 2) { // Small Pellet
                SDL_SetRenderDrawColor(renderer, 255, 184, 151, 255);
                SDL_Rect p = { (x * TS) + 10, (y * TS) + 10 + hudOffset, 4, 4 };
                SDL_RenderFillRect(renderer, &p);
            } else if (maze[y][x] == 3) { // Energizer
                SDL_SetRenderDrawColor(renderer, 255, 184, 151, 255);
                SDL_Rect pp = { (x * TS) + 6, (y * TS) + 6 + hudOffset, 12, 12 };
                SDL_RenderFillRect(renderer, &pp);
            }
        }
    }
}

void render_fruit(SDL_Renderer* renderer, GameContext* ctx, SDL_Texture** fruitTextures, int hudOffset) {
    if (ctx->fruitActive != 1 || !fruitTextures)
        return;

    int texIndex = 0;
    if (ctx->level == 2) texIndex = 1;
    else if (ctx->level >= 3 && ctx->level <= 4) texIndex = 2;
    else if (ctx->level >= 5 && ctx->level <= 6) texIndex = 3;
    else if (ctx->level >= 7 && ctx->level <= 8) texIndex = 4;
    else if (ctx->level >= 9 && ctx->level <= 10) texIndex = 5;
    else if (ctx->level >= 11 && ctx->level <= 12) texIndex = 6;
    else if (ctx->level >= 13) texIndex = 7;

    if (fruitTextures[texIndex]) {
        SDL_Rect fRect = { 13 * 24, (20 * 24) + hudOffset, 24, 24 };
        SDL_RenderCopy(renderer, fruitTextures[texIndex], NULL, &fRect);
    }
}

void render_hud(SDL_Renderer* renderer, TTF_Font* font, TTF_Font* fontSmall, int score, int highScore, const char* initials, int lives, int level, int difficulty, int mazeIndex, int windowWidth, int windowHeight, SDL_Texture* texLife, SDL_Texture* texBanner, SDL_Texture** texFruits) {
    /* Silence unused parameters due to tracker refactor */
    (void)fontSmall;
    (void)score; (void)highScore; (void)difficulty; (void)mazeIndex;

    SDL_Color white = {255, 255, 255, 255};
    int centerX = windowWidth / 2;

    /* --- TOP HUD --- */
    render_hud_text(renderer, font, "Current Score", 16, 8, white);
    render_tracker(renderer, font, &score_hud, 16, 36, white, false);

    // Center Column: Initials, Highest Score, and Highest Round
    render_hud_text(renderer, font, initials, centerX - 130, 8, white);
    render_tracker(renderer, font, &high_score_hud, centerX - 70, 8, white, true);
    render_tracker(renderer, font, &high_level_hud, centerX - 70, 32, white, true);

    // Right Column: Trackers for Difficulty, Maze, and Round
    render_tracker(renderer, font, &difficulty_hud, windowWidth - 160, 8, white, true);
    render_tracker(renderer, font, &maze_hud, windowWidth - 160, 32, white, true);
    render_tracker(renderer, font, &level_hud, windowWidth - 160, 56, white, true);

    /* --- BOTTOM HUD --- */
    for (int i = 0; i < lives - 1; i++) {
//      SDL_Rect lifeRect = { 16 + (i * 28), windowHeight - 32, 24, 24 };
        SDL_Rect lifeRect = { 16 + (i * 28), windowHeight - 36, 24, 24 };
        SDL_RenderCopy(renderer, texLife, NULL, &lifeRect);
    }

    if (texBanner) {
        int bw, bh;
        SDL_QueryTexture(texBanner, NULL, NULL, &bw, &bh);
        int scaledH = 28;
        int scaledW = (bw * scaledH) / bh;
//      SDL_Rect bRect = { centerX - (scaledW / 2), windowHeight - 34, scaledW, scaledH };
        SDL_Rect bRect = { centerX - (scaledW / 2), windowHeight - 38, scaledW, scaledH };
        SDL_RenderCopy(renderer, texBanner, NULL, &bRect);
    }

    int startLvl = (level > 7) ? level - 6 : 1;
    int maxFruits = (level > 7) ? 7 : level;
    for (int i = 0; i < maxFruits; i++) {
        int drawLvl = startLvl + i;
        int fIdx = 0;
        if (drawLvl == 2) fIdx = 1;
        else if (drawLvl >= 3 && drawLvl <= 4) fIdx = 2;
        else if (drawLvl >= 5 && drawLvl <= 6) fIdx = 3;
        else if (drawLvl >= 7 && drawLvl <= 8) fIdx = 4;
        else if (drawLvl >= 9 && drawLvl <= 10) fIdx = 5;
        else if (drawLvl >= 11 && drawLvl <= 12) fIdx = 6;
        else if (drawLvl >= 13) fIdx = 7;

        SDL_Rect fRect = { windowWidth - 40 - ((maxFruits - 1 - i) * 28), windowHeight - 36, 24, 24 };
        SDL_RenderCopy(renderer, texFruits[fIdx], NULL, &fRect);
    }
}

void render_overlay(SDL_Renderer* renderer, TTF_Font* font, TTF_Font* fontSmall, int state, int wonLastGame, int windowWidth, int windowHeight, int mazeIndex, int difficulty, int showHelp, int hudOffset) {
    (void)mazeIndex;
    (void)difficulty;

    if (state == STATE_GAMEPLAY) return;

    SDL_Color white = {255, 255, 255, 255};
    SDL_Color gray = {150, 150, 150, 255};
    SDL_Color yellow = {255, 255, 0, 255};

    if (state == STATE_PAUSED) {
        SDL_Rect ob = { windowWidth/2 - 100, windowHeight/2 - 80 + hudOffset, 200, 80 };
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderFillRect(renderer, &ob);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(renderer, &ob);
        render_hud_text(renderer, font, "PAUSED", windowWidth/2 - 38, windowHeight/2 - 50 + hudOffset, yellow);
        return;
    }

    if (state == STATE_READY) {
        /* FLASHING READY TEXT: Blinks every 250ms */
        if ((SDL_GetTicks() / 250) % 2 == 0) {
            render_hud_text(renderer, font, "READY!", windowWidth / 2 - 38, (20 * 24) + hudOffset, yellow);
        }
        return;
    }

    if (state == STATE_START) {
        if (showHelp) {
            // --- THE HELP SCREEN ---
            SDL_Rect ob = { windowWidth/2 - 180, windowHeight/2 - 130, 360, 270 };
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderFillRect(renderer, &ob);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderDrawRect(renderer, &ob);
            render_hud_text(renderer, font, "--- GAME CONTROLS ---", windowWidth/2 - 105, windowHeight/2 - 110, yellow);
            render_hud_text(renderer, fontSmall, "Ctrl + 1 : Easy Mode (Slower Ghosts)", windowWidth/2 - 130, windowHeight/2 - 65, white);
            render_hud_text(renderer, fontSmall, "Ctrl + 2 : Normal Mode (Classic)", windowWidth/2 - 130, windowHeight/2 - 40, white);
            render_hud_text(renderer, fontSmall, "Ctrl + 3 : Hard Mode (Fast Ghosts)", windowWidth/2 - 130, windowHeight/2 - 15, white);
            render_hud_text(renderer, fontSmall, "Ctrl + M : Toggle Maze Layouts", windowWidth/2 - 130, windowHeight/2 + 10, white);
            render_hud_text(renderer, fontSmall, "P : Pause and Continue", windowWidth/2 - 130, windowHeight/2 + 35, white);
            render_hud_text(renderer, fontSmall, "Ctrl + Del : Wipe Win/Loss Stats", windowWidth/2 - 130, windowHeight/2 + 60, gray);
            render_hud_text(renderer, font, "Press F1 to close", windowWidth/2 - 80, windowHeight/2 + 100, yellow);
        } else {
            // --- STANDARD START SCREEN ---
            SDL_Rect ob = { windowWidth/2 - 175, windowHeight/2 - 70, 350, 140 };
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderFillRect(renderer, &ob);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderDrawRect(renderer, &ob);

            render_hud_text(renderer, font, "READY!", windowWidth/2 - 40, windowHeight/2 - 50, yellow);
            render_hud_text(renderer, fontSmall, "Press ENTER to Start", windowWidth/2 - 80, windowHeight/2 - 10, white);
            render_hud_text(renderer, fontSmall, "Press F1 for Help / Controls", windowWidth/2 - 105, windowHeight/2 + 30, yellow);
        }
    }
    else if (state == STATE_SCORES) {
        SDL_Rect ob = { windowWidth/2 - 150, windowHeight/2 - 50, 300, 100 };
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderFillRect(renderer, &ob);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(renderer, &ob);

        const char* mainText = wonLastGame ? "LEVEL CLEARED!" : "GAME OVER";
        render_hud_text(renderer, font, mainText, windowWidth/2 - 60, windowHeight/2 - 30, white);
        render_hud_text(renderer, fontSmall, "Press ENTER to Restart", windowWidth/2 - 85, windowHeight/2 + 10, white);
    }
}

void render_name_entry(SDL_Renderer* renderer, TTF_Font* font, TTF_Font* fontSmall, GameContext* ctx, int windowWidth, int windowHeight) {
    /* Centered popup box dimensions for you to tweak */
    int boxW = 320;
    int boxH = 180;
    SDL_Rect bg = {windowWidth/2 - boxW/2, windowHeight/2 - boxH/2, boxW, boxH};

    /* Opaque Background */
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderFillRect(renderer, &bg);

    /* Optional: Frame it with a yellow border */
    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
    SDL_RenderDrawRect(renderer, &bg);

    SDL_Color yellow = {255, 255, 0, 255};
    SDL_Color white = {255, 255, 255, 255};

    /* FIX: Spread out the Y-coordinates heavily so they don't overlap */
    render_hud_text(renderer, font, "NEW HIGH SCORE!", windowWidth/2 - 75, windowHeight/2 - 80, yellow);
    render_hud_text(renderer, fontSmall, "ENTER INITIALS", windowWidth/2 - 55, windowHeight/2 - 35, white);

    /* Render the brackets */
    render_hud_text(renderer, font, "[", windowWidth/2 - 55, windowHeight/2 + 5, white);
    render_hud_text(renderer, font, "]", windowWidth/2 + 35, windowHeight/2 + 5, white);

    /* Render each letter, highlighting the active one in yellow */
    for (int i = 0; i < 3; i++) {
        char letter[2] = {ctx->currentName[i], '\0'};
        SDL_Color letterColor = (i == ctx->nameCursorPos) ? yellow : white;

        /* Optional: Add a 250ms blink to the active letter */
        if (i == ctx->nameCursorPos && (SDL_GetTicks() / 250) % 2 != 0) {
            continue; /* Skip rendering this frame to create the blink */
        }

        render_hud_text(renderer, font, letter, windowWidth/2 - 32 + (i * 25), windowHeight/2 + 5, letterColor);
    }

    render_hud_text(renderer, fontSmall, "PRESS ENTER TO ACCEPT", windowWidth/2 - 85, windowHeight/2 + 55, white);
}

void render_intermission(SDL_Renderer* renderer, GameContext* ctx, SDL_Texture** texLeft, SDL_Texture** texRight, SDL_Texture* texBlinky, SDL_Texture* texScared, int hudOffset) {
    int groundY = 20 * 24 + hudOffset;

    if (ctx->intermissionAct == 1) {
        /* Standard 3-frame walk cycle synced to the cutscene timer */
        int animIdx = (ctx->intermissionTimer / 8) % 3;
        if (ctx->intermissionTimer < 150) {
            /* Phase 1: Blinky chases Normal Pac-Man Left */
            SDL_Rect pac = { (int)ctx->interX, groundY - 12, 24, 24 };
            SDL_RenderCopy(renderer, texLeft[animIdx], NULL, &pac);

            SDL_Rect blinky = { (int)ctx->interX + 64, groundY - 12, 24, 24 };
            SDL_RenderCopy(renderer, texBlinky, NULL, &blinky);
        } else {
            /* Phase 2: Giant Pac-Man chases Blue Ghost Right */
            SDL_Rect ghost = { (int)ctx->interX, groundY - 12, 24, 24 };
            SDL_RenderCopy(renderer, texScared, NULL, &ghost);

            /* 48x48 Hardware Scaling Trick */
            SDL_Rect giantPac = { (int)ctx->interX - 80, groundY - 36, 48, 48 };
            SDL_RenderCopy(renderer, texRight[animIdx], NULL, &giantPac);
        }
    } else if (ctx->intermissionAct == 2) {
        int animIdx = (ctx->intermissionTimer / 8) % 3;
        /* Phase 1: Chase. Phase 2: Blinky snags on a tack and flashes in shock */
        SDL_Rect pac = { (int)ctx->interX, groundY - 12, 24, 24 };
        SDL_RenderCopy(renderer, texLeft[animIdx], NULL, &pac);

        int blinkyX;
        SDL_Texture* bTex = texBlinky;
        if (ctx->intermissionTimer < 100) {
            blinkyX = (int)ctx->interX + 72;
        } else {
            /* Snag! Blinky freezes in place while Pac-Man escapes */
            blinkyX = (int)(MAP_WIDTH * 24.0f) - (int)(100 * 3.5f) + 72;
            bTex = ((ctx->intermissionTimer / 10) % 2 == 0) ? texScared : texBlinky;
        }
        SDL_Rect blinky = { blinkyX, groundY - 12, 24, 24 };
        SDL_RenderCopy(renderer, bTex, NULL, &blinky);

    } else if (ctx->intermissionAct == 3) {
        int animIdx = (ctx->intermissionTimer / 8) % 3;
        if (ctx->intermissionTimer < 150) {
            /* Phase 1: Standard Chase Left */
            SDL_Rect pac = { (int)ctx->interX, groundY - 12, 24, 24 };
            SDL_RenderCopy(renderer, texLeft[animIdx], NULL, &pac);

            SDL_Rect blinky = { (int)ctx->interX + 72, groundY - 12, 24, 24 };
            SDL_RenderCopy(renderer, texBlinky, NULL, &blinky);
        } else {
            /* Phase 2: Retreat Right! Blinky is chased by a Scared Ghost (Worm proxy) */
            SDL_Rect blinky = { (int)ctx->interX, groundY - 12, 24, 24 };
            SDL_RenderCopy(renderer, texBlinky, NULL, &blinky);

            SDL_Rect worm = { (int)ctx->interX - 64, groundY - 12, 24, 24 };
            SDL_RenderCopy(renderer, texScared, NULL, &worm);
        }
    }
}

void render_kill_screen_glitch(SDL_Renderer* renderer, SDL_Texture** texFruits, SDL_Texture* texBlinky, int hudOffset) {
    /* 1. Blackout the right half (Grid X: 14 to 27) */
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_Rect rightHalf = { 14 * 24, hudOffset, 14 * 24, 36 * 24 };
    SDL_RenderFillRect(renderer, &rightHalf);

    /* 2. Seed deterministic static based on time for authentic CRT flicker */
    srand(SDL_GetTicks() / 60);

    for (int i = 0; i < 180; i++) {
        int rx = (14 + (rand() % 14)) * 24;
        int ry = (rand() % 36) * 24 + hudOffset;

        int rVal = rand() % 5;
        if (rVal == 0) {
            int fIdx = rand() % 8;
            if (texFruits && texFruits[fIdx]) {
                SDL_Rect dest = { rx, ry, 24, 24 };
                SDL_RenderCopy(renderer, texFruits[fIdx], NULL, &dest);
            }
        } else if (rVal == 1) {
            SDL_Rect dest = { rx, ry, 24, 24 };
            SDL_RenderCopy(renderer, texBlinky, NULL, &dest);
        } else {
            /* Raw memory block corruption */
            SDL_SetRenderDrawColor(renderer, rand() % 255, rand() % 255, rand() % 255, 255);
            SDL_Rect dest = { rx + (rand() % 12), ry + (rand() % 12), 12, 12 };
            SDL_RenderFillRect(renderer, &dest);
        }
    }
}
