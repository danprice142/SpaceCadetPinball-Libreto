/*
 * SpaceCadetPinball libretro core
 * Minimal winmain stubs - no SDL window management
 */

#include "libretro_pch.h"
#include "../SpaceCadetPinball/winmain.h"
#include "../SpaceCadetPinball/options.h"

// winmain stubs for libretro
SDL_Window* winmain::MainWindow = nullptr;
SDL_Renderer* winmain::Renderer = nullptr;

bool winmain::LaunchBallEnabled = true;
bool winmain::HighScoresEnabled = true;
bool winmain::DemoActive = false;
bool winmain::single_step = false;
int winmain::MainMenuHeight = 0;

void winmain::HandleGameBinding(GameBindings binding, bool down)
{
    (void)binding;
    (void)down;
}

void winmain::Restart()
{
}

// DebugOverlay stubs
namespace DebugOverlay
{
    void UnInit() {}
    void DrawOverlay() {}
}
