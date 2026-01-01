/*
 * SpaceCadetPinball libretro core
 * Precompiled header - SDL-free, ImGui-free
 */

#ifndef LIBRETRO_PCH_H
#define LIBRETRO_PCH_H

// Prevent original pch.h from being included
#ifndef PCH_H
#define PCH_H
#endif

// Standard includes
#include <cstdio>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <type_traits>
#include <chrono>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cstring>
#include <string>
#include <thread>
#include <map>
#include <unordered_map>
#include <initializer_list>
#include <cstdlib>

// Libretro SDL/ImGui replacement types and stubs
#include "libretro_sdl_compat.h"

typedef char* LPSTR;
typedef const char* LPCSTR;

constexpr char PathSeparator =
#ifdef _WIN32
'\\';
#else
'/';
#endif

#define assertm(exp, msg) assert(((void)msg, exp))

inline size_t pgm_save(int width, int height, char* data, FILE* outfile)
{
    size_t n = 0;
    n += fprintf(outfile, "P5\n%d %d\n%d\n", width, height, 0xFF);
    n += fwrite(data, 1, width * height, outfile);
    return n;
}

inline float RandFloat()
{
    return static_cast<float>(std::rand() / static_cast<double>(RAND_MAX));
}

template <typename T>
constexpr int Sign(T val)
{
    return (T(0) < val) - (val < T(0));
}

template <typename T>
const T& Clamp(const T& n, const T& lower, const T& upper)
{
    return std::max(lower, std::min(n, upper));
}

#ifdef _WIN32
extern FILE* fopenu(const char* path, const char* opt);
#else
inline FILE* fopenu(const char* path, const char* opt)
{
    return fopen(path, opt);
}
#endif

constexpr const char* PlatformDataPaths[2] = 
{
#ifdef _WIN32
    nullptr
#else
    "/usr/local/share/SpaceCadetPinball/",
    "/usr/share/SpaceCadetPinball/"
#endif
};

constexpr float Pi = 3.14159265358979323846f;

// Bring isnan into global scope for compatibility
using std::isnan;

#endif // LIBRETRO_PCH_H
