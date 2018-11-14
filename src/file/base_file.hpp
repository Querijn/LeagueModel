#pragma once
#include <string.hpp>

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
	BaseFile(StringView a_FileName) : m_Name(a_FileName) {}

protected:
	StringView m_Name;
	size_t m_Size;
};
