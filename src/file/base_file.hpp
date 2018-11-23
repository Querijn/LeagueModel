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
	BaseFile(String a_FileName) : m_Name(a_FileName) {}

protected:
	String m_Name;
	size_t m_Size;
};
