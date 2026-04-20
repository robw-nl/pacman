#ifndef RENDER_H
#define RENDER_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "common.h"
#include "map.h"
#include "hud.h"

SDL_Texture* load_texture(const char* path, SDL_Renderer* renderer);
void render_hud(SDL_Renderer* renderer, TTF_Font* font, TTF_Font* fontSmall, int score, int highScore, const char* initials, int lives, int level, int difficulty, int mazeIndex, int windowWidth, int windowHeight, SDL_Texture* texLife, SDL_Texture* texBanner, SDL_Texture** texFruits);
void render_hud_text(SDL_Renderer* renderer, TTF_Font* font, const char* text, int x, int y, SDL_Color color);
void render_hud_right_aligned(SDL_Renderer* renderer, TTF_Font* font, HUD_Tracker* t, int windowWidth);
void render_intermission(SDL_Renderer* renderer, GameContext* ctx, SDL_Texture** texLeft, SDL_Texture** texRight, SDL_Texture* texBlinky, SDL_Texture* texScared, int hudOffset);
void render_maze(SDL_Renderer* renderer, uint8_t maze[MAP_HEIGHT][MAP_WIDTH], int hudOffset, int isFlashing);
void render_fruit(SDL_Renderer* renderer, GameContext* ctx, SDL_Texture** fruitTextures, int hudOffset);
void render_overlay(SDL_Renderer* renderer, TTF_Font* font, TTF_Font* fontSmall, int state, int wonLastGame, int windowWidth, int windowHeight, int mazeIndex, int difficulty, int showHelp, int hudOffset);
void render_name_entry(SDL_Renderer* renderer, TTF_Font* font, TTF_Font* fontSmall, GameContext* ctx, int windowWidth, int windowHeight);
void render_kill_screen_glitch(SDL_Renderer* renderer, SDL_Texture** texFruits, SDL_Texture* texBlinky, int hudOffset);
void render_tracker(SDL_Renderer* renderer, TTF_Font* font, HUD_Tracker* t, int x, int y, SDL_Color color, bool includeLabel);

void invalidate_maze_cache(void);
void invalidate_text_cache(void);

#endif
