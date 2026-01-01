/*
 * SpaceCadetPinball libretro core - Audio System
 * Handles WAV loading and mixing for libretro audio output
 */

#ifndef LIBRETRO_AUDIO_H
#define LIBRETRO_AUDIO_H

#include <cstdint>
#include <string>
#include <vector>
#include <cstring>

// Forward declaration
struct LibretroAudioChunk;

// Audio system for libretro
namespace libretro_audio {

// Initialize audio system
void init(int sample_rate, int channels);

// Shutdown audio system
void shutdown();

// Load a WAV file and return a chunk handle
LibretroAudioChunk* load_wav(const char* filename);

// Free a loaded chunk
void free_chunk(LibretroAudioChunk* chunk);

// Play a chunk on any available channel, returns channel number or -1
int play_channel(int channel, LibretroAudioChunk* chunk, int loops);

// Halt a specific channel (-1 for all)
void halt_channel(int channel);

// Pause/Resume channels
void pause_channel(int channel);
void resume_channel(int channel);

// Set volume (0-128)
void set_volume(int channel, int volume);

// Set position (angle and distance for stereo panning)
void set_position(int channel, int16_t angle, uint8_t distance);

// Allocate channels
int allocate_channels(int num_channels);

// Check if channel is playing
int is_playing(int channel);

// Mix and get audio samples for libretro
// Returns number of frames written
int get_audio_samples(int16_t* buffer, int num_frames);

// Check if audio is initialized
bool is_initialized();

} // namespace libretro_audio

// Audio chunk structure
struct LibretroAudioChunk {
    int16_t* samples;       // Audio samples (stereo interleaved)
    uint32_t num_samples;   // Number of sample frames
    int sample_rate;        // Original sample rate
    int channels;           // 1 = mono, 2 = stereo
    bool allocated;         // Whether samples were allocated by us
};

#endif // LIBRETRO_AUDIO_H
