#pragma once
#ifdef _WIN32

#include <platform/base_platform.hpp>

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