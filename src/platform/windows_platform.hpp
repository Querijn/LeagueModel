#pragma once
#ifdef _WIN32

#include <platform/base_platform.hpp>

#include <Windows.h>

class WindowsPlatform
{
public:
	static bool IsNative();
	static bool IsWeb();

	size_t GetCoreCount();
	static void Sleep(size_t a_Milliseconds);
	static void SetMainLoop(LoopFunction a_Function);
	static void AtExit(AtExitFunction a_Function);

	template<typename T>
	static const char* GetTypeName()
	{
		return typeid(T).name();
	}
};

#endif