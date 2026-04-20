#ifndef COMMON_H
#define COMMON_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <stdio.h> // Ensure stdio.h is here for the FILE pointer
#include "map.h"

// Debug macro - enabled via -DDEBUG_MODE in your Makefile
#ifdef DEBUG_MODE
extern char current_debug_log[256];
// Existing file logger
#define DEBUG_LOG(fmt, ...) \
do { \
    FILE *f = fopen(current_debug_log, "a"); \
    if (f) { \
        fprintf(f, "[%ld] " fmt "\n", (long)SDL_GetTicks(), ##__VA_ARGS__); \
        fclose(f); \
    } \
} while (0)

// New terminal output
#define DEBUG_PRINT(fmt, ...) fprintf(stderr, "[Debug Output] " fmt "\n", ##__VA_ARGS__)
#else
#define DEBUG_LOG(fmt, ...)
#define DEBUG_PRINT(fmt, ...) do {} while (0)
#endif

#pragma once

#define COLLISION_RADIUS_GHOST 10.0f
#define COLLISION_RADIUS_FRUIT 12.0f

#ifdef DEBUG_MODE
#define START_LEVEL 19
#else
#define START_LEVEL 1
#endif

// 1. The State Machine
typedef enum {
    STATE_START,
    STATE_READY,    // This is now properly guarded
    STATE_GAMEPLAY,
    STATE_DEATH,
    STATE_MENU,
    STATE_PAUSED,
    STATE_SCORES,
    STATE_EXIT,
    STATE_INTERMISSION,
    STATE_NAME_ENTRY,
    STATE_KILL_SCREEN
} GameState;

// 2. Session Stats and Game Settings
typedef struct {
    int difficulty;
    int mazeIndex;
    int highScores[3][TOTAL_MAZES];
    int highLevels[3][TOTAL_MAZES];
    char initials[3][TOTAL_MAZES][4];
} GameStats;

typedef enum {
    MODE_SCATTER,
    MODE_CHASE
} GhostMode;

typedef struct {
    float x, y;
    int gridX, gridY;
    int dirX, dirY;
    int nextDirX, nextDirY;
    float speed;
    int frightenedTimer;
    int eatPauseTimer;
} Player;

typedef struct {
    float x, y;
    int gridX, gridY;
    int dirX, dirY;
    float speed;
    float baseSpeed;
    int isDead;
    int isImmune;
    int reviveTimer;
} Ghost;

typedef enum {
    CH_SIREN      = 0,
    CH_DEATH      = 1,
    CH_MUNCH      = 2,
    CH_EAT_GHOST  = 3,
    CH_START      = 4,
    CH_FRUIT      = 5,
    CH_EXTRA_LIFE = 6,
    CH_POWER      = 7
} AudioChannel;

// Enable audio
typedef struct {
    Mix_Chunk* siren;
    Mix_Chunk* death;
    Mix_Chunk* munch;
    Mix_Chunk* eatGhost;
    Mix_Chunk* start;
    Mix_Chunk* eatFruit;
    Mix_Chunk* extraLife;
    Mix_Chunk* powerPellet;
} AudioAssets;

// ghostsEaten and extraLifeAwarded
typedef struct {
    AudioAssets sfx;
    int score;
    int level;
    int lives;
    int fruitActive;
    int fruitTimer;
    int ghostsEaten;
    int extraLifeAwarded;
    int state;
    int wonLastGame;
    int pelletsRemaining;
    int totalPellets;
    int difficulty;
    int extraLifeThreshold;
    int idleTimer;
    int deathTimer;
    int isLocked[4];
    int houseCounters[4];

    int useGlobalCounter;
    int globalPelletCounter;

    /* Pause / Freeze State */
    int freezeTimer;
    int eatenGhostIndex;
    int lastEatenScore;
    int lastFruitScore;

    /* Intermission State */
    int intermissionAct;
    int intermissionTimer;
    float interX;
    float interY;

    int killScreenTimer;
    int killScreenShown;

    /* Name Entry State */
    int nameCursorPos;
    char currentName[4];
    int transitionTimer;
    int showHelp;
    int isDemo;
    int demoTimer;
    unsigned int engineTicks;
} GameContext;

#endif
