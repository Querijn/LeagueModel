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
	BaseFile(const std::string& a_FileName) : Name(a_FileName) {}

protected:
	std::string Name;
	size_t m_Size;
};
