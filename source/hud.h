#ifndef HUD_H
#define HUD_H

#include <stdbool.h>
#include <SDL2/SDL.h>

typedef struct {
    char label[32];
    int value;
    float x, y;
    bool dirty;
    SDL_Texture* texture;
} HUD_Tracker;

extern HUD_Tracker difficulty_hud;
extern HUD_Tracker maze_hud;
extern HUD_Tracker score_hud;
extern HUD_Tracker high_score_hud;
extern HUD_Tracker level_hud;
extern HUD_Tracker high_level_hud;

void update_tracker(HUD_Tracker *t, int new_val);

#endif
