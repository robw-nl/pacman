#include "stats.h"
#include <string.h>
#include <stdio.h>

#include "stats.h"
#include <string.h>
#include <stdio.h>

void save_stats(const GameStats* stats) {
    FILE* f = fopen("pacman_stats.tmp", "wb");
    if (f) {
        fwrite(stats, sizeof(GameStats), 1, f);
        fclose(f);
        rename("pacman_stats.tmp", "pacman_stats.dat");
    }
}

void load_stats(GameStats* stats) {
    memset(stats, 0, sizeof(GameStats));
    FILE* f = fopen("pacman_stats.dat", "rb");
    if (f) {
        if (fread(stats, sizeof(GameStats), 1, f) == 1) {
            // Validate difficulty range
            if (stats->difficulty < 1 || stats->difficulty > 3) {
                stats->difficulty = 2;
                stats->mazeIndex = 0;
                save_stats(stats);
            }
        }
        fclose(f);
    } else {
        // Default initialization if file missing
        stats->difficulty = 2;
        stats->mazeIndex = 0;
        save_stats(stats);
    }
}
