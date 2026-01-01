/*
 * SpaceCadetPinball libretro game wrapper
 * Implements pinball_game interface using the original game code
 * Compiled with libretro compatibility headers (no SDL, no ImGui)
 */

// Use libretro compatibility headers
#include "libretro_pch.h"

// Include game headers - these will use the stub SDL/ImGui definitions
#include "../SpaceCadetPinball/pb.h"
#include "../SpaceCadetPinball/render.h"
#include "../SpaceCadetPinball/gdrv.h"
#include "../SpaceCadetPinball/options.h"
#include "../SpaceCadetPinball/fullscrn.h"
#include "../SpaceCadetPinball/nudge.h"
#include "../SpaceCadetPinball/timer.h"
#include "../SpaceCadetPinball/score.h"
#include "../SpaceCadetPinball/high_score.h"
#include "../SpaceCadetPinball/control.h"
#include "../SpaceCadetPinball/translations.h"
#include "../SpaceCadetPinball/midi.h"
#include "../SpaceCadetPinball/Sound.h"
#include "../SpaceCadetPinball/TPinballTable.h"
#include "../SpaceCadetPinball/TTextBox.h"
#include "../SpaceCadetPinball/winmain.h"
#include "../SpaceCadetPinball/partman.h"
#include "../SpaceCadetPinball/loader.h"
#include "../SpaceCadetPinball/proj.h"
#include "../SpaceCadetPinball/GroupData.h"
#include "../SpaceCadetPinball/TBall.h"

#include <cstdio>
#include <cstring>
#include <string>
#include "libretro_audio.h"


// Global log callback for libretro logging
static void (*g_log_cb)(const char* msg) = nullptr;

void set_log_callback(void (*cb)(const char*)) {
    g_log_cb = cb;
}

static void log_debug(const char* msg) {
    if (g_log_cb) g_log_cb(msg);
}

namespace pinball_game {

static bool initialized = false;
static bool paused = false;

bool init(const char* dat_path, const char* base_path_str)
{
    if (initialized)
        return true;

    pb::BasePath = base_path_str ? base_path_str : "./";
    
    std::string path_str = dat_path ? dat_path : "";
    size_t last_sep = path_str.find_last_of("/\\");
    if (last_sep != std::string::npos)
        pb::DatFileName = path_str.substr(last_sep + 1);
    else
        pb::DatFileName = path_str;

    std::string upper_name = pb::DatFileName;
    for (auto& c : upper_name)
        c = static_cast<char>(toupper(c));
    
    pb::FullTiltMode = (upper_name != "PINBALL.DAT");
    pb::FullTiltDemoMode = (upper_name == "DEMO.DAT");

    options::Options.Sounds = true;
    options::Options.Music = false;
    options::Options.FullScreen = false;
    options::Options.Players = 1;
    options::Options.Resolution = 0;
    options::Options.UniformScaling = true;
    options::Options.LinearFiltering = false;
    options::Options.ShowMenu = false;
    options::Options.FramesPerSecond = 60;
    options::Options.UpdatesPerSecond = 120;
    options::Options.SoundChannels = 8;
    options::Options.SoundVolume = 100;
    options::Options.SoundStereo = true;

    options::InitPrimary();
    high_score::read();
    
    libretro_audio::init(44100, options::Options.SoundChannels);
    Sound::Init(true, options::Options.SoundChannels, options::Options.Sounds, options::Options.SoundVolume);

    if (pb::DatFileName.empty())
        return false;
    
    auto dataFilePath = pb::make_path_name(pb::DatFileName);
    
    FILE* testFile = fopen(dataFilePath.c_str(), "rb");
    if (testFile) {
        fclose(testFile);
    } else {
        dataFilePath = dat_path;
    }
    
    pb::record_table = partman::load_records(dataFilePath.c_str(), pb::FullTiltMode);
    if (!pb::record_table)
        return false;
    
    auto useBmpFont = 0;
    pb::get_rc_int(Msg::TextBoxUseBitmapFont, &useBmpFont);
    if (useBmpFont)
        score::load_msg_font("pbmsg_ft");
    
    auto plt = (ColorRgba*)pb::record_table->field_labeled("background", FieldTypes::Palette);
    gdrv::display_palette(plt);
    
    auto backgroundBmp = pb::record_table->GetBitmap(pb::record_table->record_labeled("background"));
    if (!backgroundBmp)
        return false;
    
    auto cameraInfoId = pb::record_table->record_labeled("camera_info") + fullscrn::GetResolution();
    auto cameraInfo = (float*)pb::record_table->field(cameraInfoId, FieldTypes::FloatArray);
    auto resInfo = &fullscrn::resolution_array[fullscrn::GetResolution()];
    
    if (cameraInfo) {
        float projMat[12];
        memcpy(&projMat, cameraInfo, sizeof(float) * 4 * 3);
        cameraInfo += 12;
        auto projCenterX = resInfo->TableWidth * 0.5f;
        auto projCenterY = resInfo->TableHeight * 0.5f;
        proj::init(projMat, cameraInfo[0], projCenterX, projCenterY, cameraInfo[1], cameraInfo[2]);
    }
    
    render::init(nullptr, resInfo->TableWidth, resInfo->TableHeight);
    gdrv::copy_bitmap(render::vscreen, backgroundBmp->Width, backgroundBmp->Height,
        backgroundBmp->XPosition, backgroundBmp->YPosition, backgroundBmp, 0, 0);
    
    loader::loadfrom(pb::record_table);
    pb::mode_change(GameModes::InGame);
    
    pb::time_ticks = 0;
    timer::init(150);
    score::init();
    
    pb::MainTable = new TPinballTable();
    if (!pb::MainTable || pb::MainTable->BallList.empty())
        return false;
    
    auto ball = pb::MainTable->BallList.at(0);
    if (!ball)
        return false;
    
    pb::BallMaxSpeed = ball->Radius * 200.0f;
    pb::BallHalfRadius = ball->Radius * 0.5f;
    pb::BallToBallCollisionDistance = (ball->Radius + pb::BallHalfRadius) * 2.0f;

    fullscrn::init();
    pb::reset_table();
    pb::firsttime_setup();
    pb::replay_level(false);

    initialized = true;
    paused = false;
    Sound::Enable(true);
    
    return true;
}

void uninit()
{
    if (!initialized)
        return;
    
    pb::uninit();
    initialized = false;
    paused = false;
}

void reset()
{
    if (!initialized)
        return;
    
    pb::reset_table();
    paused = false;
}

void frame(float dt_ms)
{
    if (!initialized || paused)
        return;

    if (dt_ms > 100.0f)
        dt_ms = 100.0f;
    if (dt_ms <= 0.0f)
        return;

    pb::frame(dt_ms);
}

void new_game()
{
    if (!initialized)
        return;

    paused = false;
    pb::replay_level(false);
}

void pause_toggle()
{
    if (!initialized)
        return;

    pb::pause_continue();
    paused = !paused;
}

void left_flipper_down()
{
    if (!initialized || paused)
        return;
    if (pb::MainTable)
        pb::MainTable->Message(MessageCode::LeftFlipperInputPressed, pb::time_now);
}

void left_flipper_up()
{
    if (!initialized || paused)
        return;
    if (pb::MainTable)
        pb::MainTable->Message(MessageCode::LeftFlipperInputReleased, pb::time_now);
}

void right_flipper_down()
{
    if (!initialized || paused)
        return;
    if (pb::MainTable)
        pb::MainTable->Message(MessageCode::RightFlipperInputPressed, pb::time_now);
}

void right_flipper_up()
{
    if (!initialized || paused)
        return;
    if (pb::MainTable)
        pb::MainTable->Message(MessageCode::RightFlipperInputReleased, pb::time_now);
}

void plunger_down()
{
    if (!initialized || paused)
        return;
    if (pb::MainTable)
        pb::MainTable->Message(MessageCode::PlungerInputPressed, pb::time_now);
}

void plunger_up()
{
    if (!initialized || paused)
        return;
    if (pb::MainTable)
        pb::MainTable->Message(MessageCode::PlungerInputReleased, pb::time_now);
}

void nudge_left()
{
    if (!initialized || paused)
        return;
    if (pb::MainTable && !pb::MainTable->TiltLockFlag)
        nudge::nudge_right();
}

void nudge_right()
{
    if (!initialized || paused)
        return;
    if (pb::MainTable && !pb::MainTable->TiltLockFlag)
        nudge::nudge_left();
}

void nudge_up()
{
    if (!initialized || paused)
        return;
    if (pb::MainTable && !pb::MainTable->TiltLockFlag)
        nudge::nudge_up();
}

void nudge_left_release()
{
    if (!initialized)
        return;
    nudge::un_nudge_right(0, nullptr);
}

void nudge_right_release()
{
    if (!initialized)
        return;
    nudge::un_nudge_left(0, nullptr);
}

void nudge_up_release()
{
    if (!initialized)
        return;
    nudge::un_nudge_up(0, nullptr);
}

uint32_t* get_framebuffer()
{
    if (!initialized)
        return nullptr;
    return render::vscreen ? reinterpret_cast<uint32_t*>(render::vscreen->BmpBufPtr1) : nullptr;
}

int get_width()
{
    if (!initialized)
        return 600;
    return render::vscreen ? render::vscreen->Width : 600;
}

int get_height()
{
    if (!initialized)
        return 416;
    return render::vscreen ? render::vscreen->Height : 416;
}

bool is_game_over()
{
    if (!initialized)
        return true;
    return pb::game_mode == GameModes::GameOver;
}

} // namespace pinball_game

// UTF-8 path adapter for Windows (required by game code)
#ifdef _WIN32
#include <windows.h>
FILE* fopenu(const char* path, const char* opt)
{
    // Convert UTF-8 to wide string
    int wpath_len = MultiByteToWideChar(CP_UTF8, 0, path, -1, nullptr, 0);
    int wopt_len = MultiByteToWideChar(CP_UTF8, 0, opt, -1, nullptr, 0);
    
    if (wpath_len <= 0 || wopt_len <= 0)
        return fopen(path, opt);
    
    wchar_t* wpath = new wchar_t[wpath_len];
    wchar_t* wopt = new wchar_t[wopt_len];
    
    MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath, wpath_len);
    MultiByteToWideChar(CP_UTF8, 0, opt, -1, wopt, wopt_len);
    
    FILE* result = _wfopen(wpath, wopt);
    
    delete[] wpath;
    delete[] wopt;
    
    return result;
}
#endif
