#pragma once

#if defined(__EMSCRIPTEN__)

#include <file/emscripten_file.hpp>
using File = EmscriptenFile;

#include <file_system/emscripten_file_system.hpp>
using FileSystem = EmscriptenFileSystem;

#else

#include <file/native_file.hpp>
using File = NativeFile;

#include <file_system/native_file_system.hpp>
using FileSystem = NativeFileSystem;

#endif

#include <file/image.hpp>