#pragma once

#include <file/base_file.hpp>
#include <helper.hpp>

#include <vector>

template<typename FileType, DerivedFrom(FileType, BaseFile)>
class BaseFileSystem
{
public:
	virtual bool FileExists(const std::string& a_FileName) = 0;
	// virtual FileType* OpenFile(const std::string& a_FileName, BaseFile::OnLoadFunction a_OnLoadFunction) = 0;

private:
	std::vector<FileType> m_Files;
};