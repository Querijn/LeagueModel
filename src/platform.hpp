#pragma once

#ifdef _WIN32

#include <league_model/platform/windows_platform.hpp>
using Platform = WindowsPlatform;

#else

#include <league_model/platform/emscripten_platform.hpp>
using Platform = EmscriptenPlatform;

#endif