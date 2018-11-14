#ifdef _WIN32
#include "platform/windows_platform.hpp"

#include <algorithm>

LARGE_INTEGER g_Frequency;
LARGE_INTEGER g_Start;

bool WindowsPlatform::IsNative()
{
	return true;
}

bool WindowsPlatform::IsWeb()
{
	return false;
}

void WindowsPlatform::SetMainLoop(LoopFunction a_Function)
{
	QueryPerformanceFrequency(&g_Frequency);
	QueryPerformanceCounter(&g_Start);

	while (a_Function())
	{

	}
}

double WindowsPlatform::GetTimeSinceStart()
{
	LARGE_INTEGER t_End;
	QueryPerformanceCounter(&t_End);

	return static_cast<double>(t_End.QuadPart - g_Start.QuadPart) / g_Frequency.QuadPart;
}

#endif