#pragma once
#if defined(__EMSCRIPTEN__)

#include <file/emscripten_file.hpp>
#include <file_system/base_file_system.hpp>

#include <string>

class EmscriptenFileSystem : public BaseFileSystem<EmscriptenFile, EmscriptenFile::OnLoadFunction>
{
public:
	bool FileExists(const std::string& a_FileName) override;

	EmscriptenFile* OpenFile(const std::string& a_FileName, EmscriptenFile::OnLoadFunction a_OnLoadFunction = nullptr) override;

private:
	std::vector<EmscriptenFile> m_Files;
};

#endif