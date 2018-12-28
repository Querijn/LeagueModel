#pragma once
#ifdef _WIN32

#include <league_model/platform/base_platform.hpp>

#include <Windows.h>

class WindowsPlatform
{
public:
	static bool IsNative();
	static bool IsWeb();

	static void SetMainLoop(LoopFunction a_Function);
	static double GetTimeSinceStart();
};

#endif