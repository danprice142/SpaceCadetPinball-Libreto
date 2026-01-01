/*
 * SpaceCadetPinball libretro core
 * SDL/ImGui compatibility layer - provides stubs and replacements
 */

#ifndef LIBRETRO_SDL_COMPAT_H
#define LIBRETRO_SDL_COMPAT_H

#include <cstdint>
#include <cstring>

// ============================================================================
// SDL Type Definitions
// ============================================================================

typedef uint8_t Uint8;
typedef uint16_t Uint16;
typedef uint32_t Uint32;
typedef int32_t Sint32;
typedef uint64_t Uint64;

// SDL_Rect replacement
struct SDL_Rect
{
    int x, y;
    int w, h;
};

struct SDL_FRect
{
    float x, y;
    float w, h;
};

// SDL_Window/Renderer stubs
typedef void SDL_Window;
typedef void SDL_Renderer;
typedef void SDL_Texture;

// SDL Event types
#define SDL_QUIT            0x100
#define SDL_KEYDOWN         0x300
#define SDL_KEYUP           0x301
#define SDL_MOUSEMOTION     0x400
#define SDL_MOUSEBUTTONDOWN 0x401
#define SDL_MOUSEBUTTONUP   0x402
#define SDL_MOUSEWHEEL      0x403
#define SDL_WINDOWEVENT     0x200
#define SDL_JOYDEVICEADDED  0x605
#define SDL_JOYDEVICEREMOVED 0x606
#define SDL_CONTROLLERBUTTONDOWN 0x650
#define SDL_CONTROLLERBUTTONUP   0x651

// SDL Button definitions
#define SDL_BUTTON_LEFT   1
#define SDL_BUTTON_MIDDLE 2
#define SDL_BUTTON_RIGHT  3

// SDL Key definitions (subset used by game)
#define SDLK_UNKNOWN    0
#define SDLK_RETURN     13
#define SDLK_ESCAPE     27
#define SDLK_SPACE      32
#define SDLK_UP         1073741906
#define SDLK_DOWN       1073741905
#define SDLK_RIGHT      1073741903
#define SDLK_LEFT       1073741904
#define SDLK_z          122
#define SDLK_SLASH      47
#define SDLK_LSHIFT     1073742049
#define SDLK_RSHIFT     1073742053
#define SDLK_LCTRL      1073742048
#define SDLK_RCTRL      1073742052
#define SDLK_F1         1073741882
#define SDLK_F2         1073741883
#define SDLK_F3         1073741884
#define SDLK_F10        1073741891
#define SDLK_F4         1073741885
#define SDLK_F5         1073741886
#define SDLK_F6         1073741887
#define SDLK_F7         1073741888
#define SDLK_F8         1073741889
#define SDLK_F9         1073741890
#define SDLK_F11        1073741892
#define SDLK_F12        1073741893

// SDL Controller buttons
#define SDL_CONTROLLER_BUTTON_A             0
#define SDL_CONTROLLER_BUTTON_B             1
#define SDL_CONTROLLER_BUTTON_X             2
#define SDL_CONTROLLER_BUTTON_Y             3
#define SDL_CONTROLLER_BUTTON_BACK          4
#define SDL_CONTROLLER_BUTTON_GUIDE         5
#define SDL_CONTROLLER_BUTTON_START         6
#define SDL_CONTROLLER_BUTTON_LEFTSTICK     7
#define SDL_CONTROLLER_BUTTON_RIGHTSTICK    8
#define SDL_CONTROLLER_BUTTON_LEFTSHOULDER  9
#define SDL_CONTROLLER_BUTTON_RIGHTSHOULDER 10
#define SDL_CONTROLLER_BUTTON_DPAD_UP       11
#define SDL_CONTROLLER_BUTTON_DPAD_DOWN     12
#define SDL_CONTROLLER_BUTTON_DPAD_LEFT     13
#define SDL_CONTROLLER_BUTTON_DPAD_RIGHT    14

// SDL Mouse buttons for wheel
#define SDL_BUTTON_X1 4
#define SDL_BUTTON_X2 5

// SDL Messagebox flags
#define SDL_MESSAGEBOX_ERROR       0x00000010
#define SDL_MESSAGEBOX_WARNING     0x00000020
#define SDL_MESSAGEBOX_INFORMATION 0x00000040

// SDL_mixer - use libretro audio system
#define MIX_MAX_VOLUME 128
struct LibretroAudioChunk;
typedef LibretroAudioChunk Mix_Chunk;
typedef void Mix_Music;

// SDL texture access
#define SDL_TEXTUREACCESS_STATIC    0
#define SDL_TEXTUREACCESS_STREAMING 1
#define SDL_TEXTUREACCESS_TARGET    2

// SDL blend modes
#define SDL_BLENDMODE_NONE  0
#define SDL_BLENDMODE_BLEND 1

// SDL version macros
#define SDL_MAJOR_VERSION 2
#define SDL_MINOR_VERSION 0
#define SDL_PATCHLEVEL 0
#define SDL_MIXER_MAJOR_VERSION 2
#define SDL_MIXER_MINOR_VERSION 0
#define SDL_MIXER_PATCHLEVEL 0
#define SDL_VERSION_ATLEAST(x, y, z) (1)
#define SDL_VERSIONNUM(x, y, z) ((x)*1000 + (y)*100 + (z))

// MIX_INIT flags
#define MIX_INIT_MID 0
constexpr int MIX_INIT_MID_Proxy = MIX_INIT_MID;

// ============================================================================
// ImGui Type Definitions and Stubs
// ============================================================================

typedef uint32_t ImU32;
typedef unsigned int ImGuiID;

#define IM_COL32(R, G, B, A) (((ImU32)(A) << 24) | ((ImU32)(B) << 16) | ((ImU32)(G) << 8) | ((ImU32)(R)))
#define IM_FREE(ptr) free(ptr)

struct ImVec2
{
    float x, y;
    ImVec2() : x(0), y(0) {}
    ImVec2(float _x, float _y) : x(_x), y(_y) {}
};

struct ImVec4
{
    float x, y, z, w;
    ImVec4() : x(0), y(0), z(0), w(0) {}
    ImVec4(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}
};

// Forward declare and define ImWchar early
typedef unsigned short ImWchar;

// ImVector template (needed before ImFontAtlas)
template<typename T>
struct ImVector
{
    T* Data;
    int Size;
    int Capacity;
    
    ImVector() : Data(nullptr), Size(0), Capacity(0) {}
    void push_back(const T&) {}
    void clear() { Size = 0; }
    void resize(int) {}
};

struct ImFont {};

struct ImFontAtlas
{
    static void* DecompressCompressedStbData(const void*, unsigned, unsigned&) { return nullptr; }
    static void* DecompressCompressedBase85Data(const char*) { return nullptr; }
    bool AddFontFromFileTTF(const char*, float, void*, const unsigned short*) { return false; }
    void Build() {}
    const ImWchar* GetGlyphRangesDefault() { return nullptr; }
};

struct ImGuiIO
{
    float FontGlobalScale;
    ImFontAtlas* Fonts;
    const char* IniFilename;
    ImGuiIO() : FontGlobalScale(1.0f), Fonts(nullptr), IniFilename(nullptr) {}
};

struct ImFontGlyphRangesBuilder
{
    void AddText(const char*) {}
    void AddRanges(const ImWchar*) {}
    void BuildRanges(ImVector<ImWchar>*) {}
};

struct ImDrawData {};
struct ImGuiContext {};
struct ImGuiTextBuffer 
{
    void append(const char*, const char* = nullptr) {}
    void appendf(const char*, ...) {}
};
struct ImGuiSettingsHandler {
    const char* TypeName;
    unsigned TypeHash;
    void* UserData;
    void (*ClearAllFn)(ImGuiContext*, ImGuiSettingsHandler*);
    void (*ReadInitFn)(ImGuiContext*, ImGuiSettingsHandler*);
    void* (*ReadOpenFn)(ImGuiContext*, ImGuiSettingsHandler*, const char*);
    void (*ReadLineFn)(ImGuiContext*, ImGuiSettingsHandler*, void*, const char*);
    void (*ApplyAllFn)(ImGuiContext*, ImGuiSettingsHandler*);
    void (*WriteAllFn)(ImGuiContext*, ImGuiSettingsHandler*, ImGuiTextBuffer*);
};

struct ImVector_ImWchar : ImVector<ImWchar> {};

struct ImFontConfig {};

// ImGui stub functions
namespace ImGui
{
    inline ImGuiContext* CreateContext() { return nullptr; }
    inline void DestroyContext(ImGuiContext* = nullptr) {}
    inline ImGuiIO& GetIO() { static ImGuiIO io; return io; }
    inline void StyleColorsDark() {}
    inline void NewFrame() {}
    inline void Render() {}
    inline ImDrawData* GetDrawData() { return nullptr; }
    inline bool Begin(const char*, bool* = nullptr, int = 0) { return false; }
    inline void End() {}
    inline bool BeginMainMenuBar() { return false; }
    inline void EndMainMenuBar() {}
    inline bool BeginMenuBar() { return false; }
    inline void EndMenuBar() {}
    inline bool BeginMenu(const char*, bool = true) { return false; }
    inline void EndMenu() {}
    inline bool MenuItem(const char*, const char* = nullptr, bool = false, bool = true) { return false; }
    inline bool MenuItem(const char*, const char*, bool*, bool = true) { return false; }
    inline void Separator() {}
    inline void Text(const char*, ...) {}
    inline void TextUnformatted(const char*, const char* = nullptr) {}
    inline bool Button(const char*, ImVec2 = ImVec2(0, 0)) { return false; }
    inline bool SliderInt(const char*, int*, int, int, const char* = "%d", int = 0) { return false; }
    inline bool SliderFloat(const char*, float*, float, float, const char* = "%.3f", int = 0) { return false; }
    inline bool DragFloat(const char*, float*, float = 1.0f, float = 0, float = 0, const char* = "%.3f", int = 0) { return false; }
    inline void SameLine(float = 0, float = -1) {}
    inline void Image(void*, ImVec2, ImVec2 = ImVec2(0, 0), ImVec2 = ImVec2(1, 1), ImVec4 = ImVec4(1, 1, 1, 1), ImVec4 = ImVec4(0, 0, 0, 0)) {}
    inline bool BeginPopupModal(const char*, bool* = nullptr, int = 0) { return false; }
    inline void EndPopup() {}
    inline void OpenPopup(const char*, int = 0) {}
    inline void CloseCurrentPopup() {}
    inline void SetItemDefaultFocus() {}
    inline void SetNextWindowPos(ImVec2, int = 0, ImVec2 = ImVec2(0, 0)) {}
    inline bool IsWindowAppearing() { return false; }
    inline void SetKeyboardFocusHere(int = 0) {}
    inline void FocusWindow(void*) {}
    inline ImVec2 GetWindowSize() { return ImVec2(0, 0); }
    inline void* GetMainViewport() { return nullptr; }
    inline void PushStyleColor(int, ImVec4) {}
    inline void PushStyleColor(int, unsigned int) {}
    inline void PopStyleColor(int = 1) {}
    inline void PushStyleVar(int, float) {}
    inline void PopStyleVar(int = 1) {}
    inline void SetMouseCursor(int) {}
    inline bool IsPopupOpen(const char*, int = 0) { return false; }
    inline bool BeginTable(const char*, int, int = 0, ImVec2 = ImVec2(0,0), float = 0) { return false; }
    inline void EndTable() {}
    inline void TableSetupColumn(const char*, int = 0, float = 0, unsigned = 0) {}
    inline void TableHeadersRow() {}
    inline void TableNextRow(int = 0, float = 0) {}
    inline bool TableNextColumn() { return false; }
    inline void PushItemWidth(float) {}
    inline void PopItemWidth() {}
    inline bool InputText(const char*, char*, size_t, int = 0, void* = nullptr, void* = nullptr) { return false; }
    inline bool Checkbox(const char*, bool*) { return false; }
    inline void Spacing() {}
    inline void Dummy(ImVec2) {}
    inline float GetTextLineHeight() { return 12.0f; }
    inline float GetTextLineHeightWithSpacing() { return 14.0f; }
    inline ImVec2 GetContentRegionAvail() { return ImVec2(100, 100); }
    inline void SetCursorPosX(float) {}
    inline void SetCursorPosY(float) {}
    inline float GetCursorPosX() { return 0; }
    inline float GetCursorPosY() { return 0; }
    inline void Columns(int = 1, const char* = nullptr, bool = true) {}
    inline void NextColumn() {}
    inline void TreePush(const char*) {}
    inline void TreePop() {}
    inline bool TreeNode(const char*, ...) { return false; }
    inline void Indent(float = 0) {}
    inline void Unindent(float = 0) {}
    inline void PushID(int) {}
    inline void PushID(const char*) {}
    inline void PopID() {}
    inline ImGuiContext* GetCurrentContext() { return nullptr; }
    inline void AddSettingsHandler(const ImGuiSettingsHandler*) {}
    inline void SetNextWindowSize(ImVec2, int = 0) {}
    inline void SetWindowFontScale(float) {}
    inline void LoadIniSettingsFromDisk(const char*) {}
    inline void TextWrapped(const char*, ...) {}
}

inline unsigned ImHashStr(const char*, size_t = 0, unsigned = 0) { return 0; }

#define IMGUI_CHECKVERSION()
#define IMGUI_VERSION "stub"
#define ImGuiWindowFlags_HorizontalScrollbar 0
#define ImGuiWindowFlags_MenuBar 0
#define ImGuiWindowFlags_AlwaysAutoResize 0
#define ImGuiConfigFlags_NavEnableKeyboard 0
#define ImGuiConfigFlags_NavEnableGamepad 0
#define ImGuiCond_Always 0
#define ImGuiSliderFlags_AlwaysClamp 0
#define ImGuiCol_MenuBarBg 0
#define ImGuiCol_WindowBg 0
#define ImGuiStyleVar_WindowBorderSize 0
#define ImGuiMouseCursor_None 0
#define ImGuiTableFlags_Borders 0
#define ImGuiInputTextFlags_EnterReturnsTrue 0
#define ImGuiInputTextFlags_AutoSelectAll 0
#define IM_ARRAYSIZE(arr) (sizeof(arr)/sizeof((arr)[0]))
#define ImGuiCol_Text 0
#define ImGuiWindowFlags int
#define ImGuiWindowFlags_NoDecoration 0
#define ImGuiWindowFlags_NoBackground 0
#define ImGuiWindowFlags_NoMove 0
#define ImGuiWindowFlags_NoScrollWithMouse 0
#define ImGuiWindowFlags_NoSavedSettings 0
#define ImGuiWindowFlags_NoBringToFrontOnFocus 0
#define ImGuiWindowFlags_NoFocusOnAppearing 0
#define ImGuiWindowFlags_NoInputs 0
#define ImGuiWindowFlags_NoNav 0

constexpr const char* ImGuiRender = "libretro";

inline void ImGui_Render_Init(SDL_Renderer*) {}
inline void ImGui_Render_Shutdown() {}
inline void ImGui_Render_NewFrame() {}
inline void ImGui_Render_RenderDrawData(ImDrawData*) {}
inline void ImGui_ImplSDL2_InitForSDLRenderer(SDL_Window*, SDL_Renderer*) {}
inline void ImGui_ImplSDL2_Shutdown() {}
inline void ImGui_ImplSDL2_NewFrame() {}
inline bool ImGui_ImplSDL2_ProcessEvent(const void*) { return false; }

// ============================================================================
// SDL Function Stubs
// ============================================================================

inline int SDL_Init(Uint32) { return 0; }
inline void SDL_Quit() {}
inline void SDL_SetMainReady() {}
inline const char* SDL_GetError() { return ""; }
inline void SDL_ClearError() {}

inline SDL_Window* SDL_CreateWindow(const char*, int, int, int, int, Uint32) { return nullptr; }
inline void SDL_DestroyWindow(SDL_Window*) {}
inline void SDL_ShowWindow(SDL_Window*) {}
inline void SDL_SetWindowSize(SDL_Window*, int, int) {}
inline void SDL_SetWindowTitle(SDL_Window*, const char*) {}
inline void SDL_SetWindowGrab(SDL_Window*, int) {}
inline void SDL_GetWindowSize(SDL_Window*, int* w, int* h) { if(w) *w = 800; if(h) *h = 600; }
inline void SDL_WarpMouseInWindow(SDL_Window*, int, int) {}
inline int SDL_SetWindowFullscreen(SDL_Window*, Uint32) { return 0; }
inline int SDL_GetRendererOutputSize(SDL_Renderer*, int* w, int* h) { if(w) *w = 800; if(h) *h = 600; return 0; }

#define SDL_WINDOW_FULLSCREEN_DESKTOP 0x00001001
#define SDL_HINT_RENDER_SCALE_QUALITY "SDL_RENDER_SCALE_QUALITY"

inline SDL_Renderer* SDL_CreateRenderer(SDL_Window*, int, Uint32) { return nullptr; }
inline void SDL_DestroyRenderer(SDL_Renderer*) {}
inline int SDL_RenderClear(SDL_Renderer*) { return 0; }
inline int SDL_RenderCopy(SDL_Renderer*, SDL_Texture*, const SDL_Rect*, const SDL_Rect*) { return 0; }
inline int SDL_RenderCopyF(SDL_Renderer*, SDL_Texture*, const SDL_Rect*, const SDL_FRect*) { return 0; }
inline int SDL_RenderFillRect(SDL_Renderer*, const SDL_Rect*) { return 0; }
inline void SDL_RenderPresent(SDL_Renderer*) {}
inline int SDL_SetRenderDrawColor(SDL_Renderer*, Uint8, Uint8, Uint8, Uint8) { return 0; }
inline int SDL_GetRendererInfo(SDL_Renderer*, void*) { return -1; }

inline SDL_Texture* SDL_CreateTexture(SDL_Renderer*, Uint32, int, int, int) { return nullptr; }
inline void SDL_DestroyTexture(SDL_Texture*) {}
inline int SDL_SetTextureBlendMode(SDL_Texture*, int) { return 0; }
inline int SDL_LockTexture(SDL_Texture*, const SDL_Rect*, void**, int*) { return -1; }
inline void SDL_UnlockTexture(SDL_Texture*) {}
inline int SDL_UpdateTexture(SDL_Texture*, const SDL_Rect*, const void*, int) { return 0; }

inline int SDL_SetHint(const char*, const char*) { return 0; }
inline const char* SDL_GetHint(const char*) { return nullptr; }

inline char* SDL_GetPrefPath(const char*, const char*) { return nullptr; }
inline char* SDL_GetBasePath() { return nullptr; }
inline void SDL_free(void*) {}

inline Uint32 SDL_GetMouseState(int*, int*) { return 0; }

inline int SDL_PollEvent(void*) { return 0; }
inline int SDL_WaitEventTimeout(void*, int) { return 0; }
inline int SDL_PushEvent(void*) { return 0; }

inline int SDL_ShowSimpleMessageBox(Uint32, const char*, const char*, SDL_Window*) { return 0; }

inline int SDL_GameControllerAddMappingsFromRW(void*, int) { return 0; }
inline void* SDL_RWFromMem(void*, int) { return nullptr; }
inline int SDL_IsGameController(int) { return 0; }
inline void* SDL_GameControllerOpen(int) { return nullptr; }
inline void SDL_GameControllerClose(void*) {}
inline void* SDL_GameControllerFromInstanceID(int) { return nullptr; }

// SDL_mixer - implemented in libretro_audio.cpp
namespace libretro_audio {
    void init(int sample_rate, int channels);
    void shutdown();
    LibretroAudioChunk* load_wav(const char* filename);
    void free_chunk(LibretroAudioChunk* chunk);
    int play_channel(int channel, LibretroAudioChunk* chunk, int loops);
    void halt_channel(int channel);
    void pause_channel(int channel);
    void resume_channel(int channel);
    void set_volume(int channel, int volume);
    void set_position(int channel, int16_t angle, uint8_t distance);
    int allocate_channels(int num_channels);
    int is_playing(int channel);
    bool is_initialized();
}

// SDL_mixer function declarations - implemented in libretro_audio.cpp
int Mix_Init(int flags);
void Mix_Quit();
int Mix_OpenAudio(int freq, Uint16 format, int channels, int chunksize);
void Mix_CloseAudio();
int Mix_AllocateChannels(int n);
int Mix_PlayChannel(int ch, Mix_Chunk* chunk, int loops);
void Mix_HaltChannel(int ch);
int Mix_Volume(int ch, int vol);
void Mix_SetPosition(int ch, Sint32 angle, Uint8 dist);
Mix_Chunk* Mix_LoadWAV_RW(void* src, int freesrc);
void Mix_FreeChunk(Mix_Chunk* chunk);
Mix_Chunk* Mix_LoadWAV(const char* file);
void Mix_Pause(int ch);
void Mix_Resume(int ch);
int Mix_Playing(int ch);

typedef int16_t Sint16;
inline Mix_Music* Mix_LoadMUS(const char*) { return nullptr; }
inline void Mix_FreeMusic(Mix_Music*) {}
inline int Mix_PlayMusic(Mix_Music*, int) { return -1; }
inline void Mix_HaltMusic() {}
inline int Mix_VolumeMusic(int) { return 0; }
inline int Mix_PlayingMusic() { return 0; }
inline void* Mix_LoadMUS_RW(void*, int) { return nullptr; }
inline void* SDL_RWFromFile(const char*, const char*) { return nullptr; }

#define MIX_DEFAULT_FREQUENCY 44100
#define MIX_DEFAULT_FORMAT 0x8010

#define SDL_INIT_TIMER 0x00000001
#define SDL_INIT_AUDIO 0x00000010
#define SDL_INIT_VIDEO 0x00000020
#define SDL_INIT_EVENTS 0x00004000
#define SDL_INIT_JOYSTICK 0x00000200
#define SDL_INIT_GAMECONTROLLER 0x00002000

#define SDL_WINDOWPOS_UNDEFINED 0x1FFF0000
#define SDL_WINDOW_HIDDEN 0x00000008
#define SDL_WINDOW_RESIZABLE 0x00000020

#define SDL_RENDERER_ACCELERATED 0x00000002
#define SDL_RENDERER_SOFTWARE 0x00000001

#define SDL_PIXELFORMAT_BGRA32 0x16862004
#define SDL_PIXELFORMAT_ARGB8888 0x16362004

#define SDL_FALSE 0
#define SDL_TRUE 1

// SDL Event structure stub
struct SDL_KeyboardEvent { int repeat; struct { int sym; } keysym; };
struct SDL_MouseButtonEvent { int button; int x; int y; };
struct SDL_MouseMotionEvent { int x; int y; };
struct SDL_WindowEvent { int event; };
struct SDL_ControllerButtonEvent { int button; };
struct SDL_JoyDeviceEvent { int which; };

union SDL_Event {
    Uint32 type;
    SDL_KeyboardEvent key;
    SDL_MouseButtonEvent button;
    SDL_MouseMotionEvent motion;
    SDL_WindowEvent window;
    SDL_ControllerButtonEvent cbutton;
    SDL_JoyDeviceEvent jdevice;
};

// SDL timing stubs
inline Uint32 SDL_GetTicks() { return 0; }
inline Uint64 SDL_GetPerformanceFrequency() { return 1000000; }
inline Uint64 SDL_GetPerformanceCounter() { return 0; }

// SDL window event types
#define SDL_WINDOWEVENT_FOCUS_GAINED 12
#define SDL_WINDOWEVENT_FOCUS_LOST 13
#define SDL_WINDOWEVENT_TAKE_FOCUS 14
#define SDL_WINDOWEVENT_SHOWN 1
#define SDL_WINDOWEVENT_HIDDEN 2
#define SDL_WINDOWEVENT_SIZE_CHANGED 5
#define SDL_WINDOWEVENT_RESIZED 6

#endif // LIBRETRO_SDL_COMPAT_H
