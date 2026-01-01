/*
 * SpaceCadetPinball libretro core
 * Minimal high_score implementation - no ImGui
 */

#include "libretro_pch.h"
#include "../SpaceCadetPinball/high_score.h"

high_score_struct high_score::highscore_table[5];
bool high_score::ShowDialog = false;

int high_score::read()
{
    // Initialize with default high scores
    for (int i = 0; i < 5; i++)
    {
        strcpy(highscore_table[i].Name, "Player");
        highscore_table[i].Score = (5 - i) * 100000;
    }
    return 0;
}

int high_score::write()
{
    // No-op for libretro - could save to retroarch save system
    return 0;
}

int high_score::get_score_position(int score)
{
    for (int i = 0; i < 5; i++)
    {
        if (score > highscore_table[i].Score)
            return i;
    }
    return -1;
}

void high_score::show_high_score_dialog()
{
    // No ImGui dialog in libretro
}

void high_score::show_and_set_high_score_dialog(high_score_entry entry)
{
    if (entry.Position < 0 || entry.Position >= 5)
        return;
    
    // Just insert the score directly without dialog
    for (int i = 4; i > entry.Position; i--)
    {
        highscore_table[i] = highscore_table[i - 1];
    }
    highscore_table[entry.Position] = entry.Entry;
}

void high_score::RenderHighScoreDialog()
{
    // No ImGui rendering in libretro
}
