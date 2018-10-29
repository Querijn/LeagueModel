#pragma once
#if defined(_WIN32)

#include <file/native_file.hpp>
#include <file_system/base_file_system.hpp>

#include <string>

class NativeFileSystem : public BaseFileSystem<NativeFile>
{
public:
	bool FileExists(const std::string& a_FileName) override;

	NativeFile* OpenFile(const std::string& a_FileName, NativeFile::OnLoadFunction a_OnLoadFunction = nullptr);

private:
	std::vector<NativeFile> m_Files;
};

#endif