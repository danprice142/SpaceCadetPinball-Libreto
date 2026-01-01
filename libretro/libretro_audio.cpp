/*
 * SpaceCadetPinball libretro core - Audio System Implementation
 */

#include "libretro_audio.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <algorithm>

// Debug logging
static void (*g_log_cb)(const char*) = nullptr;

void set_audio_log_callback(void (*cb)(const char*)) {
    g_log_cb = cb;
}

static void audio_log(const char* msg) {
    if (g_log_cb) g_log_cb(msg);
}

namespace libretro_audio {

// Channel state
struct ChannelState {
    LibretroAudioChunk* chunk;
    double position_frac;   // Current playback position (fractional for resampling)
    int volume;             // 0-128
    int16_t angle;          // Stereo angle (0-360)
    uint8_t distance;       // Distance (0-255)
    bool playing;
    bool paused;
    int loops;              // -1 = infinite, 0 = play once, >0 = play n+1 times
};

static bool initialized = false;
static int output_sample_rate = 44100;
static int num_channels = 8;
static std::vector<ChannelState> channels;
static int master_volume = 128;

void init(int sample_rate, int num_ch) {
    output_sample_rate = sample_rate;
    num_channels = num_ch > 0 ? num_ch : 8;
    channels.resize(num_channels);
    for (auto& ch : channels) {
        ch.chunk = nullptr;
        ch.position_frac = 0.0;
        ch.volume = 128;
        ch.angle = 0;
        ch.distance = 0;
        ch.playing = false;
        ch.paused = false;
        ch.loops = 0;
    }
    initialized = true;
}

void shutdown() {
    channels.clear();
    initialized = false;
}

bool is_initialized() {
    return initialized;
}

// Simple WAV file loader
LibretroAudioChunk* load_wav(const char* filename) {
    if (!filename) return nullptr;
    
    FILE* file = fopen(filename, "rb");
    if (!file)
        return nullptr;
    
    // Read WAV header
    char riff[4];
    if (fread(riff, 1, 4, file) != 4 || memcmp(riff, "RIFF", 4) != 0) {
        fclose(file);
        return nullptr;
    }
    
    uint32_t file_size;
    fread(&file_size, 4, 1, file);
    
    char wave[4];
    if (fread(wave, 1, 4, file) != 4 || memcmp(wave, "WAVE", 4) != 0) {
        fclose(file);
        return nullptr;
    }
    
    // Find fmt chunk
    uint16_t audio_format = 0;
    uint16_t num_file_channels = 0;
    uint32_t sample_rate = 0;
    uint16_t bits_per_sample = 0;
    
    while (!feof(file)) {
        char chunk_id[4];
        uint32_t chunk_size;
        
        if (fread(chunk_id, 1, 4, file) != 4) break;
        if (fread(&chunk_size, 4, 1, file) != 1) break;
        
        if (memcmp(chunk_id, "fmt ", 4) == 0) {
            fread(&audio_format, 2, 1, file);
            fread(&num_file_channels, 2, 1, file);
            fread(&sample_rate, 4, 1, file);
            uint32_t byte_rate;
            fread(&byte_rate, 4, 1, file);
            uint16_t block_align;
            fread(&block_align, 2, 1, file);
            fread(&bits_per_sample, 2, 1, file);
            
            // Skip any extra fmt bytes
            if (chunk_size > 16) {
                fseek(file, chunk_size - 16, SEEK_CUR);
            }
        }
        else if (memcmp(chunk_id, "data", 4) == 0) {
            // Found data chunk
            if (audio_format != 1) { // Not PCM
                fclose(file);
                return nullptr;
            }
            
            // Allocate chunk
            LibretroAudioChunk* chunk = new LibretroAudioChunk();
            chunk->sample_rate = sample_rate;
            chunk->channels = num_file_channels;
            
            // Calculate number of sample frames
            int bytes_per_sample = bits_per_sample / 8;
            uint32_t num_frames = chunk_size / (bytes_per_sample * num_file_channels);
            chunk->num_samples = num_frames;
            
            // Allocate stereo output buffer
            chunk->samples = new int16_t[num_frames * 2];
            chunk->allocated = true;
            
            // Read and convert samples
            if (bits_per_sample == 8) {
                uint8_t* temp = new uint8_t[chunk_size];
                fread(temp, 1, chunk_size, file);
                
                for (uint32_t i = 0; i < num_frames; i++) {
                    int16_t left, right;
                    if (num_file_channels == 1) {
                        left = right = (int16_t)((temp[i] - 128) * 256);
                    } else {
                        left = (int16_t)((temp[i * 2] - 128) * 256);
                        right = (int16_t)((temp[i * 2 + 1] - 128) * 256);
                    }
                    chunk->samples[i * 2] = left;
                    chunk->samples[i * 2 + 1] = right;
                }
                delete[] temp;
            }
            else if (bits_per_sample == 16) {
                int16_t* temp = new int16_t[chunk_size / 2];
                fread(temp, 2, chunk_size / 2, file);
                
                for (uint32_t i = 0; i < num_frames; i++) {
                    if (num_file_channels == 1) {
                        chunk->samples[i * 2] = temp[i];
                        chunk->samples[i * 2 + 1] = temp[i];
                    } else {
                        chunk->samples[i * 2] = temp[i * 2];
                        chunk->samples[i * 2 + 1] = temp[i * 2 + 1];
                    }
                }
                delete[] temp;
            }
            else {
                // Unsupported bit depth
                delete[] chunk->samples;
                delete chunk;
                fclose(file);
                return nullptr;
            }
            
            fclose(file);
            return chunk;
        }
        else {
            // Skip unknown chunk
            fseek(file, chunk_size, SEEK_CUR);
        }
    }
    
    fclose(file);
    return nullptr;
}

void free_chunk(LibretroAudioChunk* chunk) {
    if (chunk) {
        if (chunk->allocated && chunk->samples) {
            delete[] chunk->samples;
        }
        delete chunk;
    }
}

int play_channel(int channel, LibretroAudioChunk* chunk, int loops) {
    if (!initialized || !chunk)
        return -1;
    
    // Find available channel if -1
    int ch = channel;
    if (ch < 0) {
        for (int i = 0; i < (int)channels.size(); i++) {
            if (!channels[i].playing) {
                ch = i;
                break;
            }
        }
        if (ch < 0) {
            // All channels busy, use oldest
            ch = 0;
        }
    }
    
    if (ch >= (int)channels.size()) return -1;
    
    channels[ch].chunk = chunk;
    channels[ch].position_frac = 0.0;
    channels[ch].playing = true;
    channels[ch].paused = false;
    channels[ch].loops = loops;
    
    return ch;
}

void halt_channel(int channel) {
    if (!initialized) return;
    
    if (channel < 0) {
        for (auto& ch : channels) {
            ch.playing = false;
            ch.paused = false;
        }
    } else if (channel < (int)channels.size()) {
        channels[channel].playing = false;
        channels[channel].paused = false;
    }
}

void pause_channel(int channel) {
    if (!initialized) return;
    
    if (channel < 0) {
        for (auto& ch : channels) {
            if (ch.playing) ch.paused = true;
        }
    } else if (channel < (int)channels.size()) {
        if (channels[channel].playing) channels[channel].paused = true;
    }
}

void resume_channel(int channel) {
    if (!initialized) return;
    
    if (channel < 0) {
        for (auto& ch : channels) {
            ch.paused = false;
        }
    } else if (channel < (int)channels.size()) {
        channels[channel].paused = false;
    }
}

void set_volume(int channel, int volume) {
    if (!initialized) return;
    
    volume = std::max(0, std::min(128, volume));
    
    if (channel < 0) {
        master_volume = volume;
    } else if (channel < (int)channels.size()) {
        channels[channel].volume = volume;
    }
}

void set_position(int channel, int16_t angle, uint8_t distance) {
    if (!initialized) return;
    if (channel < 0 || channel >= (int)channels.size()) return;
    
    channels[channel].angle = angle;
    channels[channel].distance = distance;
}

int allocate_channels(int num_ch) {
    if (!initialized) return 0;
    
    num_channels = num_ch > 0 ? num_ch : 8;
    channels.resize(num_channels);
    for (size_t i = 0; i < channels.size(); i++) {
        if (!channels[i].playing) {
            channels[i].chunk = nullptr;
            channels[i].volume = 128;
        }
    }
    return num_channels;
}

int is_playing(int channel) {
    if (!initialized) return 0;
    
    if (channel < 0) {
        int count = 0;
        for (const auto& ch : channels) {
            if (ch.playing && !ch.paused) count++;
        }
        return count;
    }
    
    if (channel < (int)channels.size()) {
        return channels[channel].playing && !channels[channel].paused ? 1 : 0;
    }
    return 0;
}

int get_audio_samples(int16_t* buffer, int num_frames) {
    if (!initialized || !buffer) return 0;
    
    memset(buffer, 0, num_frames * 2 * sizeof(int16_t));
    
    // Mix all playing channels
    for (auto& ch : channels) {
        if (!ch.playing || ch.paused || !ch.chunk) continue;
        
        LibretroAudioChunk* chunk = ch.chunk;
        
        // Calculate volume with distance attenuation
        float vol_scale = (float)ch.volume / 128.0f * (float)master_volume / 128.0f;
        float dist_scale = 1.0f - (float)ch.distance / 255.0f;
        vol_scale *= dist_scale;
        
        // Calculate stereo panning from angle
        float angle_rad = (float)ch.angle * 3.14159f / 180.0f;
        float left_pan = 0.5f - 0.5f * sinf(angle_rad);
        float right_pan = 0.5f + 0.5f * sinf(angle_rad);
        
        int left_vol = (int)(vol_scale * left_pan * 256);
        int right_vol = (int)(vol_scale * right_pan * 256);
        
        // Calculate sample rate conversion ratio
        // Source is typically 11025Hz, output is 44100Hz
        double rate_ratio = (double)chunk->sample_rate / (double)output_sample_rate;
        
        for (int i = 0; i < num_frames; i++) {
            // Calculate source position with resampling
            double src_pos_f = ch.position_frac + i * rate_ratio;
            uint32_t src_pos = (uint32_t)src_pos_f;
            
            if (src_pos >= chunk->num_samples) {
                if (ch.loops == 0) {
                    ch.playing = false;
                    break;
                } else if (ch.loops > 0) {
                    ch.loops--;
                }
                // Reset position
                ch.position_frac = 0.0;
                src_pos_f = i * rate_ratio;
                src_pos = (uint32_t)src_pos_f;
                if (src_pos >= chunk->num_samples) {
                    ch.playing = false;
                    break;
                }
            }
            
            // Linear interpolation for smoother resampling
            double frac = src_pos_f - src_pos;
            uint32_t next_pos = src_pos + 1;
            if (next_pos >= chunk->num_samples) next_pos = src_pos;
            
            int16_t left1 = chunk->samples[src_pos * 2];
            int16_t right1 = chunk->samples[src_pos * 2 + 1];
            int16_t left2 = chunk->samples[next_pos * 2];
            int16_t right2 = chunk->samples[next_pos * 2 + 1];
            
            int16_t left = (int16_t)(left1 + frac * (left2 - left1));
            int16_t right = (int16_t)(right1 + frac * (right2 - right1));
            
            // Mix with volume
            int mixed_left = buffer[i * 2] + ((left * left_vol) >> 8);
            int mixed_right = buffer[i * 2 + 1] + ((right * right_vol) >> 8);
            
            // Clamp
            buffer[i * 2] = (int16_t)std::max(-32768, std::min(32767, mixed_left));
            buffer[i * 2 + 1] = (int16_t)std::max(-32768, std::min(32767, mixed_right));
        }
        
        // Update fractional position for next call
        ch.position_frac += num_frames * rate_ratio;
        // Wrap if we looped
        while (ch.position_frac >= chunk->num_samples && ch.playing) {
            ch.position_frac -= chunk->num_samples;
        }
    }
    
    return num_frames;
}

} // namespace libretro_audio

// SDL_mixer compatible functions - these are called by Sound.cpp

int Mix_Init(int) { return 0; }

void Mix_Quit() { 
    libretro_audio::shutdown(); 
}

int Mix_OpenAudio(int freq, uint16_t, int channels, int) { 
    libretro_audio::init(freq, channels);
    return 0;
}

void Mix_CloseAudio() { 
    libretro_audio::shutdown(); 
}

int Mix_AllocateChannels(int n) { 
    return libretro_audio::allocate_channels(n); 
}

int Mix_PlayChannel(int ch, LibretroAudioChunk* chunk, int loops) {
    return libretro_audio::play_channel(ch, chunk, loops);
}

void Mix_HaltChannel(int ch) { 
    libretro_audio::halt_channel(ch); 
}

int Mix_Volume(int ch, int vol) { 
    libretro_audio::set_volume(ch, vol); 
    return vol; 
}

void Mix_SetPosition(int ch, int32_t angle, uint8_t dist) { 
    libretro_audio::set_position(ch, (int16_t)angle, dist); 
}

LibretroAudioChunk* Mix_LoadWAV_RW(void*, int) { 
    return nullptr; 
}

void Mix_FreeChunk(LibretroAudioChunk* chunk) { 
    libretro_audio::free_chunk(chunk); 
}

LibretroAudioChunk* Mix_LoadWAV(const char* file) { 
    return libretro_audio::load_wav(file); 
}

void Mix_Pause(int ch) { 
    libretro_audio::pause_channel(ch); 
}

void Mix_Resume(int ch) { 
    libretro_audio::resume_channel(ch); 
}

int Mix_Playing(int ch) { 
    return libretro_audio::is_playing(ch); 
}
