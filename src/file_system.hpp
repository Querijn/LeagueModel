#pragma once

#if defined(__EMSCRIPTEN__)

#include <file/emscripten_file.hpp>
using File = EmscriptenFile;

#else

#include <file/native_file.hpp>
using File = NativeFile;

#endif

class FileSystem
{
public:
	static File* GetFile(StringView a_FilePath);
	static void CloseFile(File& a_File);
};