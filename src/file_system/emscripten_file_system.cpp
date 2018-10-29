#if defined(__EMSCRIPTEN__)
#include "emscripten_file_system.hpp"

bool EmscriptenFileSystem::FileExists(const std::string& a_FileName)
{
	return true;
}

EmscriptenFile * EmscriptenFileSystem::OpenFile(const std::string& a_FileName, EmscriptenFile::OnLoadFunction a_OnLoadFunction)
{
	m_Files.push_back(EmscriptenFile(a_FileName, a_OnLoadFunction));
	return &m_Files.back();
}
#endif