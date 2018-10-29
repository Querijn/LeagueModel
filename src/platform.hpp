#pragma once

#ifdef _WIN32

#include <platform/windows_platform.hpp>
using Platform = WindowsPlatform;

#else

#include <platform/emscripten_platform.hpp>
using Platform = EmscriptenPlatform;

#endif