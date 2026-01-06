/*
 * SpaceCadetPinball libretro core
 * Software Rendering Implementation
 * No SDL, No ImGui - pure libretro
 */

#include "libretro.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <vector>
#include <string>
#include <cstdint>

// Log callback setter from libretro_game.cpp
extern void set_log_callback(void (*cb)(const char*));

// Forward declarations for game interface (implemented in libretro_game.cpp)
namespace pinball_game {
    bool init(const char* dat_path, const char* base_path);
    void uninit();
    void reset();
    void frame(float dt_ms);
    void new_game();
    void pause_toggle();
    
    void left_flipper_down();
    void left_flipper_up();
    void right_flipper_down();
    void right_flipper_up();
    void plunger_down();
    void plunger_up();
    void nudge_left();
    void nudge_right();
    void nudge_up();
    void nudge_left_release();
    void nudge_right_release();
    void nudge_up_release();
    
    uint32_t* get_framebuffer();
    int get_width();
    int get_height();
    bool is_game_over();
}

// Libretro callbacks
static retro_environment_t environ_cb = nullptr;
static retro_video_refresh_t video_cb = nullptr;
static retro_audio_sample_t audio_cb = nullptr;
static retro_audio_sample_batch_t audio_batch_cb = nullptr;
static retro_input_poll_t input_poll_cb = nullptr;
static retro_input_state_t input_state_cb = nullptr;
static retro_log_printf_t log_cb = nullptr;

// Simple log wrapper for game code
static void game_log_callback(const char* msg) {
    if (log_cb)
        log_cb(RETRO_LOG_INFO, "[game] %s\n", msg);
}

// Game state
static bool game_loaded = false;
static std::string game_path;
static std::string base_path;
static std::string system_dir;
static std::string save_dir;

// Video dimensions
static unsigned game_width = 600;
static unsigned game_height = 416;
static const float GAME_FPS = 60.0f;
static const float GAME_SAMPLE_RATE = 44100.0f;

// Audio buffer
static std::vector<int16_t> audio_buffer;
static const size_t AUDIO_BUFFER_SIZE = 4096;

// Previous input state for edge detection
static bool prev_left_flipper = false;
static bool prev_right_flipper = false;
static bool prev_plunger = false;
static bool prev_nudge_left = false;
static bool prev_nudge_right = false;
static bool prev_nudge_up = false;
static bool prev_start = false;
static bool prev_select = false;

#define LOG_INFO(...) do { if (log_cb) log_cb(RETRO_LOG_INFO, __VA_ARGS__); } while(0)
#define LOG_WARN(...) do { if (log_cb) log_cb(RETRO_LOG_WARN, __VA_ARGS__); } while(0)
#define LOG_ERROR(...) do { if (log_cb) log_cb(RETRO_LOG_ERROR, __VA_ARGS__); } while(0)

static void update_input(void);
static void process_audio(void);

static void update_input(void)
{
    if (!input_poll_cb || !input_state_cb)
        return;

    input_poll_cb();

    bool start = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START) != 0;
    bool select = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT) != 0;
    
    bool left_flipper = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L) != 0 ||
                        input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT) != 0;
    bool right_flipper = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R) != 0 ||
                         input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT) != 0;
    bool plunger = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A) != 0 ||
                   input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN) != 0;
    bool nudge_left = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2) != 0;
    bool nudge_right = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2) != 0;
    bool nudge_up = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP) != 0 ||
                    input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B) != 0;

    // Start - new game
    if (start && !prev_start && pinball_game::is_game_over())
        pinball_game::new_game();
    prev_start = start;

    // Select - pause
    if (select && !prev_select)
        pinball_game::pause_toggle();
    prev_select = select;

    // Left flipper
    if (left_flipper && !prev_left_flipper)
        pinball_game::left_flipper_down();
    else if (!left_flipper && prev_left_flipper)
        pinball_game::left_flipper_up();
    prev_left_flipper = left_flipper;

    // Right flipper
    if (right_flipper && !prev_right_flipper)
        pinball_game::right_flipper_down();
    else if (!right_flipper && prev_right_flipper)
        pinball_game::right_flipper_up();
    prev_right_flipper = right_flipper;

    // Plunger
    if (plunger && !prev_plunger)
        pinball_game::plunger_down();
    else if (!plunger && prev_plunger)
        pinball_game::plunger_up();
    prev_plunger = plunger;

    // Nudge
    if (nudge_left && !prev_nudge_left)
        pinball_game::nudge_left();
    else if (!nudge_left && prev_nudge_left)
        pinball_game::nudge_left_release();
    prev_nudge_left = nudge_left;

    if (nudge_right && !prev_nudge_right)
        pinball_game::nudge_right();
    else if (!nudge_right && prev_nudge_right)
        pinball_game::nudge_right_release();
    prev_nudge_right = nudge_right;

    if (nudge_up && !prev_nudge_up)
        pinball_game::nudge_up();
    else if (!nudge_up && prev_nudge_up)
        pinball_game::nudge_up_release();
    prev_nudge_up = nudge_up;
}

// Forward declare audio functions
namespace libretro_audio {
    int get_audio_samples(int16_t* buffer, int num_frames);
    bool is_initialized();
}
extern void set_audio_log_callback(void (*cb)(const char*));

static void process_audio(void)
{
    size_t frames = (size_t)(GAME_SAMPLE_RATE / GAME_FPS);
    audio_buffer.resize(frames * 2, 0);
    
    if (libretro_audio::is_initialized()) {
        libretro_audio::get_audio_samples(audio_buffer.data(), (int)frames);
    }
    
    if (audio_batch_cb)
        audio_batch_cb(audio_buffer.data(), frames);
}

// Libretro API
extern "C" {

RETRO_API void retro_set_environment(retro_environment_t cb)
{
    environ_cb = cb;

    struct retro_log_callback log;
    if (cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log))
        log_cb = log.log;

    bool no_content = true;
    cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_content);

    enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_XRGB8888;
    cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt);

    static const struct retro_input_descriptor desc[] = {
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,      "Left Flipper" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,      "Right Flipper" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,   "Left Flipper (Alt)" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT,  "Right Flipper (Alt)" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,      "Plunger / Launch Ball" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,   "Plunger (Alt)" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2,     "Nudge Left" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2,     "Nudge Right" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,     "Nudge Up" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,      "Nudge Up (Alt)" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,  "New Game" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Pause" },
        { 0, 0, 0, 0, nullptr }
    };
    cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, (void*)desc);

    LOG_INFO("Environment set\n");
}

RETRO_API void retro_set_video_refresh(retro_video_refresh_t cb) { video_cb = cb; }
RETRO_API void retro_set_audio_sample(retro_audio_sample_t cb) { audio_cb = cb; }
RETRO_API void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) { audio_batch_cb = cb; }
RETRO_API void retro_set_input_poll(retro_input_poll_t cb) { input_poll_cb = cb; }
RETRO_API void retro_set_input_state(retro_input_state_t cb) { input_state_cb = cb; }

RETRO_API void retro_init(void)
{
    LOG_INFO("retro_init\n");
    
    set_log_callback(game_log_callback);
    set_audio_log_callback(game_log_callback);

    const char* sys_dir = nullptr;
    if (environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &sys_dir) && sys_dir)
        system_dir = sys_dir;

    const char* sav_dir = nullptr;
    if (environ_cb(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY, &sav_dir) && sav_dir)
        save_dir = sav_dir;

    audio_buffer.reserve(AUDIO_BUFFER_SIZE);
}

RETRO_API void retro_deinit(void)
{
    LOG_INFO("retro_deinit\n");
    
    if (game_loaded)
    {
        pinball_game::uninit();
        game_loaded = false;
    }

    audio_buffer.clear();
}

RETRO_API unsigned retro_api_version(void) { return RETRO_API_VERSION; }

RETRO_API void retro_get_system_info(struct retro_system_info *info)
{
    memset(info, 0, sizeof(*info));
    info->library_name = "Space Cadet Pinball";
    info->library_version = "2.1.0";
    info->valid_extensions = "dat";
    info->need_fullpath = true;
    info->block_extract = false;
}

RETRO_API void retro_get_system_av_info(struct retro_system_av_info *info)
{
    memset(info, 0, sizeof(*info));
    
    unsigned w = game_loaded ? pinball_game::get_width() : 600;
    unsigned h = game_loaded ? pinball_game::get_height() : 416;
    
    info->geometry.base_width = w;
    info->geometry.base_height = h;
    info->geometry.max_width = w;
    info->geometry.max_height = h;
    info->geometry.aspect_ratio = (float)w / (float)h;
    
    info->timing.fps = GAME_FPS;
    info->timing.sample_rate = GAME_SAMPLE_RATE;
}

RETRO_API void retro_set_controller_port_device(unsigned port, unsigned device)
{
    (void)port; (void)device;
}

RETRO_API void retro_reset(void)
{
    LOG_INFO("retro_reset\n");
    if (game_loaded)
    {
        pinball_game::reset();
        pinball_game::new_game();
    }
}

RETRO_API void retro_run(void)
{
    if (!game_loaded)
        return;

    update_input();

    float dt = 1000.0f / GAME_FPS;
    pinball_game::frame(dt);

    uint32_t* fb = pinball_game::get_framebuffer();
    if (fb)
    {
        video_cb(fb, pinball_game::get_width(), pinball_game::get_height(), 
                 pinball_game::get_width() * sizeof(uint32_t));
    }

    process_audio();
}

RETRO_API size_t retro_serialize_size(void) { return 0; }
RETRO_API bool retro_serialize(void *data, size_t size) { (void)data; (void)size; return false; }
RETRO_API bool retro_unserialize(const void *data, size_t size) { (void)data; (void)size; return false; }
RETRO_API void retro_cheat_reset(void) {}
RETRO_API void retro_cheat_set(unsigned index, bool enabled, const char *code) { (void)index; (void)enabled; (void)code; }

RETRO_API bool retro_load_game(const struct retro_game_info *game)
{
    LOG_INFO("retro_load_game\n");

    std::string dat_file;
    
    if (game && game->path)
    {
        game_path = game->path;
        
        size_t last_sep = game_path.find_last_of("/\\");
        if (last_sep != std::string::npos)
        {
            base_path = game_path.substr(0, last_sep + 1);
            dat_file = game_path.substr(last_sep + 1);
        }
        else
        {
            base_path = "./";
            dat_file = game_path;
        }
    }
    else
    {
        base_path = system_dir.empty() ? "./SpaceCadetPinball/" : system_dir + "/SpaceCadetPinball/";
        
        const char* dat_files[] = { "PINBALL.DAT", "CADET.DAT", "pinball.dat", "cadet.dat" };
        for (const char* dat : dat_files)
        {
            std::string path = base_path + dat;
            FILE* f = fopen(path.c_str(), "rb");
            if (f)
            {
                fclose(f);
                dat_file = dat;
                break;
            }
        }
        
        if (dat_file.empty())
        {
            LOG_ERROR("Could not find game data in %s\n", base_path.c_str());
            LOG_ERROR("Please place PINBALL.DAT or CADET.DAT in the SpaceCadetPinball subdirectory\n");
            return false;
        }
    }

    LOG_INFO("Base path: %s\n", base_path.c_str());
    LOG_INFO("DAT file: %s\n", dat_file.c_str());

    std::string full_path = base_path + dat_file;
    
    FILE* test_f = fopen(full_path.c_str(), "rb");
    if (test_f) {
        fclose(test_f);
    } else {
        LOG_ERROR("Cannot open DAT file: %s\n", full_path.c_str());
        return false;
    }
    
    if (!pinball_game::init(full_path.c_str(), base_path.c_str()))
    {
        LOG_ERROR("Failed to initialize game\n");
        return false;
    }

    game_width = pinball_game::get_width();
    game_height = pinball_game::get_height();
    game_loaded = true;
    
    LOG_INFO("Game loaded: %ux%u\n", game_width, game_height);
    return true;
}

RETRO_API bool retro_load_game_special(unsigned game_type, const struct retro_game_info *info, size_t num_info)
{
    (void)game_type; (void)info; (void)num_info;
    return false;
}

RETRO_API void retro_unload_game(void)
{
    LOG_INFO("retro_unload_game\n");
    if (game_loaded)
    {
        pinball_game::uninit();
        game_loaded = false;
    }
}

RETRO_API unsigned retro_get_region(void) { return RETRO_REGION_NTSC; }
RETRO_API void *retro_get_memory_data(unsigned id) { (void)id; return nullptr; }
RETRO_API size_t retro_get_memory_size(unsigned id) { (void)id; return 0; }

} // extern "C"
