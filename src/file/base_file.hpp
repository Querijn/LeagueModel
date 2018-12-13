#pragma once
#include <string>

enum FileLoadState
{
	NotLoaded,
	NotFound,
	FailedToLoad,
	Loaded
};

class BaseFile
{
public:
	BaseFile(const std::string& a_FileName) : m_Name(a_FileName) {}

protected:
	std::string m_Name;
	size_t m_Size;
};
