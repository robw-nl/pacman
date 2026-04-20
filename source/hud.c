#include "hud.h"

// 1. Allocate the actual memory for the trackers
HUD_Tracker difficulty_hud = { "Difficulty: ", 1, 0.0f, 0.0f, true, NULL };
HUD_Tracker maze_hud       = { "Maze: ", 1, 0.0f, 0.0f, true, NULL };
HUD_Tracker level_hud      = { "Level: ", 1, 0.0f, 0.0f, true, NULL };
HUD_Tracker high_level_hud = { "Highest Level: ", 1, 0.0f, 0.0f, true, NULL };
HUD_Tracker score_hud      = { "", 0, 0.0f, 0.0f, true, NULL };
HUD_Tracker high_score_hud = { "Highest Score: ", 0, 0.0f, 0.0f, true, NULL };

// 2. Logic to update values (only sets dirty if changed)
void update_tracker(HUD_Tracker *t, int new_val) {
    if (t->value != new_val) {
        t->value = new_val;
        t->dirty = true;
    }
}
