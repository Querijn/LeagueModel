#ifdef _WIN32
#include "platform/windows_platform.hpp"

#include <algorithm>

bool WindowsPlatform::IsNative()
{
	return true;
}

bool WindowsPlatform::IsWeb()
{
	return false;
}

size_t WindowsPlatform::GetCoreCount()
{
	return 1;
}

void WindowsPlatform::Sleep(size_t a_Milliseconds)
{
	::Sleep(a_Milliseconds);
}

void WindowsPlatform::SetMainLoop(LoopFunction a_Function)
{
	while (a_Function())
	{

	}
}

void WindowsPlatform::AtExit(AtExitFunction a_Function)
{
	std::atexit(a_Function);
}

#endif