#if defined(_WIN32)
#include "native_file_system.hpp"

#include <sys/stat.h>

bool NativeFileSystem::FileExists(const std::string& a_FileName)
{
	struct stat t_Buffer;
	return stat(a_FileName.c_str(), &t_Buffer) == 0;
}

NativeFile * NativeFileSystem::OpenFile(const std::string& a_FileName, NativeFile::OnLoadFunction a_OnLoadFunction)
{
	m_Files.push_back(NativeFile(a_FileName, a_OnLoadFunction));
	return &m_Files.back();
}
#endif