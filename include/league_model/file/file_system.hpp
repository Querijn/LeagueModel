#pragma once

#if defined(__EMSCRIPTEN__)

#include <league_model/file/emscripten_file.hpp>
using File = EmscriptenFile;

#else

#include <league_model/file/native_file.hpp>
using File = NativeFile;

#endif

class FileSystem
{
public:
	static File* GetFile(const std::string& a_FilePath);
	static void CloseLoadedFiles();
};