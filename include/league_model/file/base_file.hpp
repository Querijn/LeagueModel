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

	bool IsHandled() const { return m_Handled; }

protected:
	FileLoadState m_State = FileLoadState::NotLoaded;
	std::string Name;
	size_t m_Size;
	bool m_Handled = false;
};
