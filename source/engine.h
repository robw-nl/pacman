#ifndef ENGINE_H
#define ENGINE_H

#include <SDL2/SDL.h>
#include "common.h"
#include "map.h"

void update_player(Player* p, uint8_t maze[MAP_HEIGHT][MAP_WIDTH], GameContext* ctx);
void update_blinky(Ghost* g, Player* p, uint8_t maze[MAP_HEIGHT][MAP_WIDTH], GameContext* ctx, GhostMode mode);
void update_pinky(Ghost* g, Player* p, uint8_t maze[MAP_HEIGHT][MAP_WIDTH], GameContext* ctx, GhostMode mode);
void update_inky(Ghost* g, Player* p, uint8_t maze[MAP_HEIGHT][MAP_WIDTH], GameContext* ctx, GhostMode mode);
void update_clyde(Ghost* g, Player* p, uint8_t maze[MAP_HEIGHT][MAP_WIDTH], GameContext* ctx, GhostMode mode);

void handle_pellets(Player* p, uint8_t maze[MAP_HEIGHT][MAP_WIDTH], GameContext* ctx, SDL_Renderer* renderer, int hudOffset);
void handle_fruit(Player* p, GameContext* ctx);

int check_collision(float x1, float y1, float x2, float y2, float size);
void handle_intermission(GameContext* ctx);
void reset_entities(Player* p);

#endif

