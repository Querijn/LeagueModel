#pragma once
#if defined(_WIN32)

#include <file/native_file.hpp>
#include <file_system/base_file_system.hpp>

#include <string>

class NativeFileSystem : public BaseFileSystem<NativeFile, NativeFile::OnLoadFunction>
{
public:
	bool FileExists(const std::string& a_FileName) override;

	NativeFile* OpenFile(const std::string& a_FileName, NativeFile::OnLoadFunction a_OnLoadFunction = nullptr) override;

private:
	std::vector<NativeFile> m_Files;
};

#endif