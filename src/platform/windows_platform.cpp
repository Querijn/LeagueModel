#ifdef _WIN32
#include "league_model/platform/windows_platform.hpp"

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

struct OnStart
{
	OnStart()
	{
		QueryPerformanceFrequency(&g_Frequency);
		QueryPerformanceCounter(&g_Start);
	}
};

OnStart t;

void WindowsPlatform::SetMainLoop(LoopFunction a_Function)
{
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