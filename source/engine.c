#include "engine.h"   // This already brings in common.h and map.h
#include <math.h>

/* Forward declaration for the internal AI helper */
static void execute_ghost_ai(Ghost* g, int targetX, int targetY, uint8_t maze[MAP_HEIGHT][MAP_WIDTH], int frightened, GameContext* ctx);

void update_player(Player* p, uint8_t maze[MAP_HEIGHT][MAP_WIDTH], GameContext* ctx) {
    if (p->eatPauseTimer > 0) {
        p->eatPauseTimer--;
        return;
    }

    /* CALIBRATED ATOMIC SPEEDS: Replaces old (2.0f + 8px jump) feel */
    if (ctx->level == 1) {
        p->speed = (p->frightenedTimer > 0) ? 3.0f : 2.8f;
    } else if (ctx->level >= 2 && ctx->level <= 4) {
        p->speed = (p->frightenedTimer > 0) ? 3.1f : 3.0f;
    } else if (ctx->level >= 5) {
        p->speed = 3.2f;
    } else {
        p->speed = 2.8f;
    }

    float targetX = p->gridX * 24.0f;
    float targetY = p->gridY * 24.0f;

    /* 1. INSTANT REVERSALS (Can be executed anywhere) */
    if ((p->dirX != 0 && p->nextDirX == -p->dirX) ||
        (p->dirY != 0 && p->nextDirY == -p->dirY)) {
        p->dirX = p->nextDirX;
    p->dirY = p->nextDirY;
    p->gridX += p->dirX;
    p->gridY += p->dirY;
    return;
    }

    /* 2. CORNER CUTTING (4-pixel pre-turn window) */
    int isPreTurning = 0;
    if (!ctx->isDemo && (p->dirX != 0 || p->dirY != 0)) {
        float distToX = targetX - p->x;
        float distToY = targetY - p->y;
        float dist = (p->dirX != 0) ? fabs(distToX) : fabs(distToY);

        if (dist <= 4.0f && dist > 0.0f && (p->dirX != p->nextDirX || p->dirY != p->nextDirY)) {
            int turnX = p->gridX + p->nextDirX;
            if (turnX < 0) turnX = MAP_WIDTH - 1; else if (turnX >= MAP_WIDTH) turnX = 0;
            int turnY = p->gridY + p->nextDirY;

            if (turnY >= 0 && turnY < MAP_HEIGHT && maze[turnY][turnX] != 1 && maze[turnY][turnX] != 4) {
                /* NEW: Carry over the 'dist' as momentum into the new direction */
                float leftover = dist;
                p->x = targetX;
                p->y = targetY;

                p->dirX = p->nextDirX;
                p->dirY = p->nextDirY;
                p->gridX += p->dirX;
                p->gridY += p->dirY;

                /* Apply the saved momentum in the new direction immediately */
                p->x += p->dirX * leftover;
                p->y += p->dirY * leftover;

                isPreTurning = 1;
            }
        }
    }

    /* 3. APPLY PRIMARY MOVEMENT */
    p->x += p->dirX * p->speed;
    p->y += p->dirY * p->speed;

    /* 4. DIAGONAL SLIDE (The visual corner cut) */
    if (p->dirY != 0 && p->x != targetX) {
        float diff = targetX - p->x;
        float step = p->speed;
        if (fabs(diff) < step) p->x = targetX;
        else p->x += (diff > 0) ? step : -step;
    } else if (p->dirX != 0 && p->y != targetY) {
        float diff = targetY - p->y;
        float step = p->speed;
        if (fabs(diff) < step) p->y = targetY;
        else p->y += (diff > 0) ? step : -step;
    }

    /* 5. STANDARD INTERSECTION SNAP */
    if (!isPreTurning) {
        int overshot = 0;
        float leftoverX = 0.0f, leftoverY = 0.0f;

        if (p->dirX == 1 && p->x > targetX) { overshot = 1; leftoverX = p->x - targetX; }
        else if (p->dirX == -1 && p->x < targetX) { overshot = 1; leftoverX = targetX - p->x; }
        else if (p->dirY == 1 && p->y > targetY) { overshot = 1; leftoverY = p->y - targetY; }
        else if (p->dirY == -1 && p->y < targetY) { overshot = 1; leftoverY = targetY - p->y; }

        if (overshot || (p->dirX == 0 && p->dirY == 0)) {
            p->x = targetX;
            p->y = targetY;

            if (ctx->isDemo) {
                int dx[] = {0, -1, 0, 1}, dy[] = {-1, 0, 1, 0};
                int validDirs[4], validCount = 0, backIdx = -1;
                for (int i = 0; i < 4; i++) {
                    int nGX = p->gridX + dx[i], nGY = p->gridY + dy[i];
                    if (nGX < 0) nGX = MAP_WIDTH - 1; else if (nGX >= MAP_WIDTH) nGX = 0;
                    if (nGY >= 0 && nGY < MAP_HEIGHT && maze[nGY][nGX] != 1 && maze[nGY][nGX] != 4) {
                        if (dx[i] == -p->dirX && dy[i] == -p->dirY) backIdx = i;
                        else validDirs[validCount++] = i;
                    }
                }
                if (validCount > 0) {
                    int choice = validDirs[rand() % validCount];
                    p->nextDirX = dx[choice]; p->nextDirY = dy[choice];
                } else if (backIdx != -1) {
                    p->nextDirX = dx[backIdx];
                    p->nextDirY = dy[backIdx];
                }
            }

            int turnX = p->gridX + p->nextDirX;
            if (turnX < 0) turnX = MAP_WIDTH - 1; else if (turnX >= MAP_WIDTH) turnX = 0;
            int turnY = p->gridY + p->nextDirY;
            if (turnY >= 0 && turnY < MAP_HEIGHT && maze[turnY][turnX] != 1 && maze[turnY][turnX] != 4) {
                p->dirX = p->nextDirX;
                p->dirY = p->nextDirY;
            }

            int fwdX = p->gridX + p->dirX;
            if (fwdX < 0) fwdX = MAP_WIDTH - 1; else if (fwdX >= MAP_WIDTH) fwdX = 0;
            int fwdY = p->gridY + p->dirY;
            if (fwdY >= 0 && fwdY < MAP_HEIGHT && (maze[fwdY][fwdX] == 1 || maze[fwdY][fwdX] == 4)) {
                p->dirX = 0;
                p->dirY = 0;
            }

            p->gridX += p->dirX; p->gridY += p->dirY;
            if (p->gridX < 0) { p->gridX = MAP_WIDTH - 1; p->x = p->gridX * 24.0f; }
            else if (p->gridX >= MAP_WIDTH) { p->gridX = 0; p->x = 0.0f; }

            float leftover = (leftoverX > 0.0f) ? leftoverX : leftoverY;
            p->x += p->dirX * leftover;
            p->y += p->dirY * leftover;
        }
    }
}

// Helper function for Ghost AI
static void execute_ghost_ai(Ghost* g, int targetX, int targetY, uint8_t maze[MAP_HEIGHT][MAP_WIDTH], int frightened, GameContext* ctx) {
    // --- 1. FLOATING EYES OVERRIDE ---
    if (g->isDead) {
        /* Arcade Quirk: Eyes slow down inside the house so the floor revival is visible */
        g->speed = (g->gridY >= 15 && g->gridX >= 11 && g->gridX <= 16) ? 2.0f : 4.0f;

        if (g->gridY >= 14 && g->gridY <= 17 && g->gridX >= 11 && g->gridX <= 16) {
            targetX = 13;
            targetY = 17; /* Inside: Target the floor */
        } else {
            targetX = 13;
            targetY = 14; /* Outside: Target the door */
        }
        frightened = 0;
    } else if (g->gridY == 17 && (g->gridX <= 5 || g->gridX >= 22)) {
        // --- TUNNEL PENALTY ---
        g->speed = 1.0f;
    } else {
        g->speed = g->baseSpeed;
    }

    float bTargetX = g->gridX * 24.0f;
    float bTargetY = g->gridY * 24.0f;

    /* SAFETY NET: Catch idle bounce offsets that push ghosts past their targets */
    int overshot = 0;
    if (g->dirX == 1 && g->x > bTargetX) overshot = 1;
    else if (g->dirX == -1 && g->x < bTargetX) overshot = 1;
    else if (g->dirY == 1 && g->y > bTargetY) overshot = 1;
    else if (g->dirY == -1 && g->y < bTargetY) overshot = 1;

    /* PERFECT DISTANCE CARRY-OVER: Prevents double-speed snapping bugs */
    float distToTarget = (g->dirX != 0) ? fabs(bTargetX - g->x) : fabs(bTargetY - g->y);

    if (overshot || distToTarget <= g->speed || (g->dirX == 0 && g->dirY == 0)) {
        float leftover = overshot ? g->speed : (g->speed - distToTarget);
        g->x = bTargetX;
        g->y = bTargetY;

        /* --- FINAL REVIVAL: Perfectly snapped to the floor tile --- */
        if (g->isDead && g->gridX == 13 && g->gridY == 17) {
            g->isDead = 0;
            g->isImmune = 1;
        }

        if (g->reviveTimer > 0) {
            g->reviveTimer--;
            g->dirX = 0; g->dirY = 0;
            // Authentic idle bounce while waiting
            g->y = 17.0f * 24.0f + (sinf(ctx->engineTicks * 0.08f) * 5.0f);
            return;
        }

        /* --- ARCADE TIE-BREAKER PRIORITY: UP > LEFT > DOWN > RIGHT --- */
        int dx[] = {0, -1, 0, 1}, dy[] = {-1, 0, 1, 0}, bestDir = -1;
        float minDist = 999999.0f;
        int validDirs[4];
        int validCount = 0;

        for (int i = 0; i < 4; i++) {
            int nGX = g->gridX + dx[i];
            int nGY = g->gridY + dy[i];

            if (nGX < 0) nGX = MAP_WIDTH - 1;
            else if (nGX >= MAP_WIDTH) nGX = 0;

            /* GHOST BOUNDARY: Ignore path tiles in the HUD/Score areas (Rows 0-2 and 34-35) */
            if (nGY < 3 || nGY > 33) continue;
            if (maze[nGY][nGX] == 1 || (dx[i] == -g->dirX && dy[i] == -g->dirY)) continue;

            // --- 2. THE ONE-WAY DOOR RULE ---
            if (g->isDead == 0) {
                // Alive ghosts cannot move DOWN into the door (y=15) from directly above (y=14)
                if (g->gridY == 14 && nGY == 15 && (nGX == 13 || nGX == 14)) continue;
            }

            // --- 3. THE "NO UP" RULE ---
            if (!frightened && !g->isDead && dy[i] == -1) {
                if ((g->gridX == 12 || g->gridX == 15) && (g->gridY == 14 || g->gridY == 26)) continue;
            }

            validDirs[validCount++] = i;
            float dist = (float)((targetX - nGX) * (targetX - nGX) + (targetY - nGY) * (targetY - nGY));

            if (dist < minDist) { minDist = dist; bestDir = i; }
        }

        /* PRNG Frightened Override: Priority over all targets except Eyes */
        /* BOUNDARY: Only ignore PRNG if physically sitting in the house (Rows 16-17) */
        int physicallyInHouse = (g->gridY >= 16 && g->gridY <= 17 && g->gridX >= 11 && g->gridX <= 16);

        if (frightened > 0 && validCount > 0 && !g->isDead && !physicallyInHouse && !g->isImmune) {
            /* Unique 'nudge' per ghost to prevent perfect stacking */
            int nudge = (int)((intptr_t)g % validCount);
            bestDir = validDirs[(rand() + nudge) % validCount];
        }

        if (bestDir != -1) { g->dirX = dx[bestDir]; g->dirY = dy[bestDir]; }
        else { g->dirX = -g->dirX; g->dirY = -g->dirY; }

        if (g->dirX != 0 || g->dirY != 0) { g->gridX += g->dirX; g->gridY += g->dirY; }

        if (g->gridX < 0) { g->gridX = MAP_WIDTH - 1; g->x = g->gridX * 24.0f; }
        else if (g->gridX >= MAP_WIDTH) { g->gridX = 0; g->x = 0.0f; }

        /* Apply remaining movement in the new direction */
        g->x += g->dirX * leftover;
        g->y += g->dirY * leftover;
    } else {
        g->x += g->dirX * g->speed;
        g->y += g->dirY * g->speed;
    }
}

/* --- BLINKY --- */
void update_blinky(Ghost* g, Player* p, uint8_t maze[MAP_HEIGHT][MAP_WIDTH], GameContext* ctx, GhostMode mode) {
    if (ctx->isLocked[0] && !g->isDead) {
        g->x = 13.0f * 24.0f;
        g->y = 14.0f * 24.0f + (sinf(ctx->engineTicks * 0.08f) * 5.0f);
        g->gridX = 13; g->gridY = 14;
        return;
    }

    /* GHOSTS: Slightly slower than player to allow for escapes */
    float baseSpeed = (ctx->difficulty == 1) ? 1.8f : (ctx->difficulty == 3) ? 2.8f : 2.6f;
    if (ctx->level >= 2 && ctx->level <= 4) baseSpeed += 0.2f;
    else if (ctx->level >= 5) baseSpeed += 0.4f;

    /* CRUISE ELROY LOGIC (Arcade-accurate thresholds) */
    int elroy1 = 20, elroy2 = 10;
    if (ctx->level == 2) { elroy1 = 30; elroy2 = 15; }
    else if (ctx->level >= 3 && ctx->level <= 5) { elroy1 = 40; elroy2 = 20; }
    else if (ctx->level >= 6 && ctx->level <= 8) { elroy1 = 50; elroy2 = 25; }
    else if (ctx->level >= 9 && ctx->level <= 11) { elroy1 = 60; elroy2 = 30; }
    else if (ctx->level >= 12 && ctx->level <= 14) { elroy1 = 80; elroy2 = 40; }
    else if (ctx->level >= 15 && ctx->level <= 18) { elroy1 = 100; elroy2 = 50; }
    else if (ctx->level >= 19) { elroy1 = 120; elroy2 = 60; }

    int isElroy = 0;
    if (ctx->pelletsRemaining <= elroy2) {
        isElroy = 2;
        baseSpeed += 0.4f; /* Scaled boost for 24px tiles */
    } else if (ctx->pelletsRemaining <= elroy1) {
        isElroy = 1;
        baseSpeed += 0.2f; /* Scaled boost for 24px tiles */
    }

    /* Frightened ghosts should be significantly slower */
    if (p->frightenedTimer > 0 && !g->isImmune) {
        /* Elroy remains more aggressive (65%) than standard frightened ghosts (55%) */
        g->baseSpeed = (isElroy > 0) ? (baseSpeed * 0.65f) : (baseSpeed * 0.55f);
    } else {
        g->baseSpeed = baseSpeed;
    }

    int tX, tY;
    /* EXIT HOUSE LOGIC for Blinky when reviving */
    if (g->gridY >= 15 && g->gridY <= 17 && g->gridX >= 11 && g->gridX <= 16) {
        tX = 13;
        tY = 14;
    } else if (mode == MODE_SCATTER && p->frightenedTimer == 0 && isElroy == 0) {
        tX = MAP_WIDTH - 2;
        tY = 0;
    } else {
        tX = p->gridX; tY = p->gridY;
    }
    execute_ghost_ai(g, tX, tY, maze, p->frightenedTimer, ctx);
}

/* --- PINKY --- */
void update_pinky(Ghost* g, Player* p, uint8_t maze[MAP_HEIGHT][MAP_WIDTH], GameContext* ctx, GhostMode mode) {
    if (ctx->isLocked[1] && !g->isDead) {
        int limit = (ctx->level == 1) ? 7 : 0;
        if (ctx->houseCounters[1] >= limit || ctx->idleTimer >= 240) {
            ctx->isLocked[1] = 0;
        } else {
            g->x = 13.0f * 24.0f;
            g->y = 17.0f * 24.0f + (sinf(ctx->engineTicks * 0.08f) * 5.0f);
            g->gridX = 13; g->gridY = 17;
            return;
        }
    }

    /* GHOSTS: Slightly slower than player to allow for escapes */
    float baseSpeed = (ctx->difficulty == 1) ? 1.8f : (ctx->difficulty == 3) ? 2.8f : 2.6f;
    if (ctx->level >= 2 && ctx->level <= 4) baseSpeed += 0.2f;
    else if (ctx->level >= 5) baseSpeed += 0.4f;

    /* Frightened ghosts should be significantly slower */
    if (p->frightenedTimer > 0 && !g->isImmune) {
        /* Frightened ghosts move at ~55% of current level's normal speed */
        g->baseSpeed = baseSpeed * 0.55f;
    } else {
        g->baseSpeed = baseSpeed;
    }

    int tX, tY;
    /* EXIT HOUSE LOGIC [cite: 438-441] */
    if (g->gridY >= 15 && g->gridY <= 17 && g->gridX >= 11 && g->gridX <= 16) {
        tX = 13;
        tY = 14;
    } else if (mode == MODE_SCATTER && p->frightenedTimer == 0) {
        tX = 1;
        tY = 0;
    } else {
        tX = p->gridX + (p->dirX * 4);
        tY = p->gridY + (p->dirY * 4);
        if (p->dirY == -1) tX -= 4;
    }
    execute_ghost_ai(g, tX, tY, maze, p->frightenedTimer, ctx);
}

/* --- INKY --- */
void update_inky(Ghost* g, Player* p, uint8_t maze[MAP_HEIGHT][MAP_WIDTH], GameContext* ctx, GhostMode mode) {
    extern Ghost blinky;
    if (ctx->isLocked[2] && !g->isDead) {
        int limit = (ctx->level == 1) ? ((ctx->totalPellets * 30) / 244) : 0;
        /* Only release if Pinky (Locked[1]) is already out */
        if (!ctx->isLocked[1] && (ctx->houseCounters[2] >= limit || ctx->idleTimer >= 240)) {
            ctx->isLocked[2] = 0;
        } else {
            g->x = 11.0f * 24.0f;
            g->y = 17.0f * 24.0f + (sinf(ctx->engineTicks * 0.08f) * 5.0f);
            g->gridX = 11; g->gridY = 17;
            return;
        }
    }

    /* GHOSTS: Slightly slower than player to allow for escapes */
    float baseSpeed = (ctx->difficulty == 1) ? 1.8f : (ctx->difficulty == 3) ? 2.8f : 2.6f;
    if (ctx->level >= 2 && ctx->level <= 4) baseSpeed += 0.2f;
    else if (ctx->level >= 5) baseSpeed += 0.4f;

    /* Frightened ghosts should be significantly slower */
    if (p->frightenedTimer > 0 && !g->isImmune) {
        /* Frightened ghosts move at ~55% of current level's normal speed */
        g->baseSpeed = baseSpeed * 0.55f;
    } else {
        g->baseSpeed = baseSpeed;
    }

    int tX, tY;
    /* EXIT HOUSE LOGIC [cite: 449-454] */
    if (g->gridY >= 15 && g->gridY <= 17 && g->gridX >= 11 && g->gridX <= 16) {
        tX = 13;
        tY = 14;
    } else if (mode == MODE_SCATTER && p->frightenedTimer == 0) {
        tX = MAP_WIDTH - 1;
        tY = MAP_HEIGHT - 1;
    } else {
        /* Inky's unique AI: Vector from Blinky through Pac-Man's offset [cite: 451-454] */
        int aX = p->gridX + (p->dirX * 2);
        int aY = p->gridY + (p->dirY * 2);
        if (p->dirY == -1) aX -= 2;
        tX = blinky.gridX + 2 * (aX - blinky.gridX);
        tY = blinky.gridY + 2 * (aY - blinky.gridY);
    }
    execute_ghost_ai(g, tX, tY, maze, p->frightenedTimer, ctx);
}

/* --- CLYDE: THE SHY ONE --- */
void update_clyde(Ghost* g, Player* p, uint8_t maze[MAP_HEIGHT][MAP_WIDTH], GameContext* ctx, GhostMode mode) {
    if (ctx->isLocked[3] && !g->isDead) {
        /* Scaled limit dynamically based on 244-pellet arcade standard */
        int limit = (ctx->level == 1) ? ((ctx->totalPellets * 60) / 244) : (ctx->level == 2) ? ((ctx->totalPellets * 50) / 244) : 0;

        if (!ctx->isLocked[2] && (ctx->houseCounters[3] >= limit || ctx->idleTimer >= 240)) {
            ctx->isLocked[3] = 0;
            ctx->idleTimer = 0; /* Reset timer so ghosts leave sequentially */
        } else {
            g->x = 15.0f * 24.0f;
            g->y = 17.0f * 24.0f + (sinf(ctx->engineTicks * 0.08f) * 5.0f);
            g->gridX = 15; g->gridY = 17;
            return;
        }
    }

    /* GHOSTS: Slightly slower than player to allow for escapes */
    float baseSpeed = (ctx->difficulty == 1) ? 1.8f : (ctx->difficulty == 3) ? 2.8f : 2.6f;
    if (ctx->level >= 2 && ctx->level <= 4) baseSpeed += 0.2f;
    else if (ctx->level >= 5) baseSpeed += 0.4f;

    /* Frightened ghosts should be significantly slower */
    if (p->frightenedTimer > 0 && !g->isImmune) {
        g->baseSpeed = baseSpeed * 0.55f;
    } else {
        g->baseSpeed = baseSpeed;
    }

    int tX, tY;
    /* EXIT HOUSE LOGIC */
    if (g->gridY >= 15 && g->gridY <= 17 && g->gridX >= 11 && g->gridX <= 16) {
        tX = 13;
        tY = 14;
    } else {
        float dist = sqrtf(powf(g->gridX - p->gridX, 2) + powf(g->gridY - p->gridY, 2));
        if (dist > 8.0f && p->frightenedTimer == 0 && mode == MODE_CHASE) {
            tX = p->gridX;
            tY = p->gridY;
        } else {
            tX = 0;
            tY = MAP_HEIGHT - 1;
        }
    }

    /* Executed unconditionally so he can actually exit the house! */
    execute_ghost_ai(g, tX, tY, maze, p->frightenedTimer, ctx);
}

void handle_pellets(Player* p, uint8_t maze[MAP_HEIGHT][MAP_WIDTH], GameContext* ctx, SDL_Renderer* renderer, int hudOffset) {
    (void)renderer; (void)hudOffset;
    int tile = maze[p->gridY][p->gridX];

    if (tile == 2 || tile == 3) {
        maze[p->gridY][p->gridX] = 0;
        ctx->idleTimer = 0;

        if (tile == 2) {
            ctx->score += 10;
            p->eatPauseTimer = 1; /* Increased from 1 for better "swallow" feedback */
            /* AUDIO: PELLET MUNCH */
            if (ctx->sfx.munch && !Mix_Playing(CH_MUNCH)) {
                Mix_PlayChannel(CH_MUNCH, ctx->sfx.munch, 0);
            }
        } else {
            ctx->score += 50;

            static const int frightFrames[] = {0, 360, 300, 240, 180, 120, 300, 120, 120, 60, 300, 120, 60, 60, 180, 60, 60, 0, 60, 0};
            p->frightenedTimer = (ctx->level >= 19) ? 0 : frightFrames[ctx->level];
            p->eatPauseTimer = 3;

            ctx->ghostsEaten = 0;

            if (ctx->sfx.powerPellet) {
                Mix_PlayChannel(CH_POWER, ctx->sfx.powerPellet, 0);
            }

            extern Ghost blinky, pinky, inky, clyde;
            Ghost* gList[] = {&blinky, &pinky, &inky, &clyde};
            for(int i = 0; i < 4; i++) {
                gList[i]->isImmune = 0;
                int inHouseZone = (gList[i]->gridY >= 14 && gList[i]->gridY <= 17 && gList[i]->gridX >= 11 && gList[i]->gridX <= 16);
                if (!gList[i]->isDead && !inHouseZone) {
                    gList[i]->gridX -= gList[i]->dirX;
                    gList[i]->gridY -= gList[i]->dirY;
                    gList[i]->dirX = -gList[i]->dirX;
                    gList[i]->dirY = -gList[i]->dirY;
                }
            }
        }

        /* GHOST HOUSE COUNTERS */
        if (ctx->useGlobalCounter) {
            ctx->globalPelletCounter++;
            if (ctx->globalPelletCounter >= 7 && ctx->isLocked[1]) ctx->isLocked[1] = 0;
            if (ctx->globalPelletCounter >= 17 && ctx->isLocked[2]) ctx->isLocked[2] = 0;
            if (ctx->globalPelletCounter >= 32 && ctx->isLocked[3]) {
                ctx->isLocked[3] = 0;
                ctx->useGlobalCounter = 0; /* Revert to personal counters once Clyde leaves */
            }
        } else {
            /* Personal counters only increment for the "next in line" */
            if (ctx->isLocked[1]) ctx->houseCounters[1]++;
            else if (ctx->isLocked[2]) ctx->houseCounters[2]++;
            else if (ctx->isLocked[3]) ctx->houseCounters[3]++;
        }

        /* OPTIMIZATION: Decrement existing counter instead of full scan */
        ctx->pelletsRemaining--;

        /* DYNAMIC FRUIT SPAWN: Arcade accurate (170 and 70 remaining) */
        if (ctx->pelletsRemaining == 170 || ctx->pelletsRemaining == 70) {
            ctx->fruitActive = 1;
            ctx->fruitTimer = 600;
        }

        if (ctx->extraLifeThreshold > 0 && ctx->score >= ctx->extraLifeThreshold && ctx->extraLifeAwarded == 0) {
            ctx->lives++;
            ctx->extraLifeAwarded = 1;
            /* AUDIO: EXTRA LIFE */
            if (ctx->sfx.extraLife) Mix_PlayChannel(CH_EXTRA_LIFE, ctx->sfx.extraLife, 0);
        }

        if (ctx->pelletsRemaining <= 0) {
            ctx->state = STATE_READY;
        }
    }
}

void handle_fruit(Player* p, GameContext* ctx) {
    if (!ctx->fruitActive) return;

    if (ctx->fruitActive == 2) {
        /* Timer for displaying the score */
        if (--ctx->fruitTimer <= 0) ctx->fruitActive = 0;
        return;
    }

    float fX = 13.0f * 24.0f, fY = 20.0f * 24.0f;
    if (check_collision(p->x, p->y, fX, fY, COLLISION_RADIUS_FRUIT)) {
        /* AUDIO: FRUIT EATEN */
        if (ctx->sfx.eatFruit) Mix_PlayChannel(CH_FRUIT, ctx->sfx.eatFruit, 0);
        int points = 100;

        if (ctx->level == 2) points = 300;
        else if (ctx->level >= 3 && ctx->level <= 4) points = 500;
        else if (ctx->level >= 5 && ctx->level <= 6) points = 700;
        else if (ctx->level >= 7 && ctx->level <= 8) points = 1000;
        else if (ctx->level >= 9 && ctx->level <= 10) points = 2000;
        else if (ctx->level >= 11 && ctx->level <= 12) points = 3000;
        else if (ctx->level >= 13) points = 5000;

        ctx->score += points;
        ctx->lastFruitScore = points;
        ctx->fruitActive = 2;
        /* State 2: Displaying Score */
        ctx->fruitTimer = 120;
        /* Show score for 2 seconds */

        /* ARCADE ACCURACY: Fruit scores can trigger the extra life */
        if (ctx->extraLifeThreshold > 0 && ctx->score >= ctx->extraLifeThreshold && ctx->extraLifeAwarded == 0) {
            ctx->lives++;
            ctx->extraLifeAwarded = 1;
            if (ctx->sfx.extraLife) Mix_PlayChannel(CH_EXTRA_LIFE, ctx->sfx.extraLife, 0);
        }

        #ifdef DEBUG_MODE
        DEBUG_LOG("Fruit eaten at level %d for %d points!", ctx->level, points);
        #endif
        return;
    }

    /* Timer for despawning the fruit */
    if (--ctx->fruitTimer <= 0) {
        ctx->fruitActive = 0;
    }
}

int check_collision(float x1, float y1, float x2, float y2, float size) {
    float dx = x1 - x2;
    float dy = y1 - y2;
    return (dx * dx + dy * dy) < (size * size);
}

void handle_intermission(GameContext* ctx) {
    ctx->intermissionTimer++;

    if (ctx->intermissionAct == 1) {
        if (ctx->intermissionTimer < 150) {
            ctx->interX -= 4.0f; // Phase 1: Run left
        } else if (ctx->intermissionTimer == 150) {
            ctx->interX = -64.0f; // Offscreen reset
        } else if (ctx->intermissionTimer < 300) {
            ctx->interX += 4.0f; // Phase 2: Run right
        } else {
            ctx->state = STATE_GAMEPLAY;
        }
    } else if (ctx->intermissionAct == 2) {
        /* Act 2: The Snag */
        if (ctx->intermissionTimer < 250) {
            ctx->interX -= 3.5f;
        } else {
            ctx->state = STATE_GAMEPLAY;
        }
    } else if (ctx->intermissionAct == 3) {
        /* Act 3: The Return Chase */
        if (ctx->intermissionTimer < 150) {
            ctx->interX -= 4.0f; // Phase 1: Run left
        } else if (ctx->intermissionTimer == 150) {
            ctx->interX = -64.0f; // Offscreen reset
        } else if (ctx->intermissionTimer < 300) {
            ctx->interX += 4.0f; // Phase 2: Run right
        } else {
            ctx->state = STATE_GAMEPLAY;
        }
    }
}


void reset_entities(Player* p) {
    extern Ghost blinky, pinky, inky, clyde;

    p->x = 13.0f * 24.0f; p->y = 26.0f * 24.0f; p->gridX = 13; p->gridY = 26;
    p->dirX = 0; p->dirY = 0; p->nextDirX = 0; p->nextDirY = 0;

    blinky.x = 13.0f * 24.0f; blinky.y = 14.0f * 24.0f; blinky.gridX = 13; blinky.gridY = 14;
    pinky.x  = 13.0f * 24.0f; pinky.y  = 17.0f * 24.0f; pinky.gridX  = 13; pinky.gridY  = 17;
    inky.x   = 11.0f * 24.0f; inky.y   = 17.0f * 24.0f; inky.gridX   = 11; inky.gridY   = 17;
    clyde.x  = 15.0f * 24.0f; clyde.y  = 17.0f * 24.0f; clyde.gridX  = 15; clyde.gridY  = 17;

    blinky.dirX = -1; blinky.dirY =  0;
    pinky.dirX  =  0; pinky.dirY  = -1;
    inky.dirX   =  0; inky.dirY   = -1;
    clyde.dirX  =  0; clyde.dirY  = -1;

    blinky.isDead = pinky.isDead = inky.isDead = clyde.isDead = 0;
    blinky.isImmune = pinky.isImmune = inky.isImmune = clyde.isImmune = 0;
    blinky.reviveTimer = pinky.reviveTimer = inky.reviveTimer = clyde.reviveTimer = 0;

    clyde.x = 15.5f * 24.0f;
    clyde.y = 17.0f * 24.0f;

    /* CLEAR GHOST STATUS EFFECTS ON DEATH/RESPAWN */
    blinky.isDead = 0; blinky.isImmune = 0; blinky.reviveTimer = 0;
    pinky.isDead = 0;  pinky.isImmune = 0;  pinky.reviveTimer = 0;
    inky.isDead = 0;   inky.isImmune = 0;   inky.reviveTimer = 0;
    clyde.isDead = 0;  clyde.isImmune = 0;  clyde.reviveTimer = 0;
}

