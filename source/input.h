#ifndef INPUT_H
#define INPUT_H

#include "common.h"
#include "stats.h"
#include "render.h"

// Returns 0 to signal the game should exit
int handle_input(SDL_Event* event, Player* player, GameContext* ctx, GameStats* stats, uint8_t maze[MAP_HEIGHT][MAP_WIDTH]);
void reset_game_state(Player* player, GameContext* ctx, GameStats* stats, uint8_t maze[MAP_HEIGHT][MAP_WIDTH]);

#endif
