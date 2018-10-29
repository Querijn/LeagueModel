#pragma once

#if defined(_WIN32)

#include <window/windows_window.hpp>
using Window = WindowsWindow;

#elif defined(__EMSCRIPTEN__)

#include <window/emscripten_window.hpp>
using Window = EmscriptenWindow;

#endif