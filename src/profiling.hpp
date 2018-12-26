#pragma once

#include <profiling/logger.hpp>
#include <profiling/memory.hpp>

#include <stdio.h>
#include <string>

class Profiler
{
public:
	using Logger = ProfilerLogger;
	static const int BufferSize = 1048576;

	static Profiler& Get();

#define __PROFILER_SCOPE_CLASS(ClassName, AtStart, AtEnd) class ClassName \
{ \
public: \
	ClassName() { AtStart; } \
	~ClassName() { AtEnd; } \
};

#define __PROFILER_SCOPE_CLASS1(ClassName, TypeOne, One, AtStart, AtEnd) class ClassName \
{ \
public: \
	ClassName(TypeOne One) { AtStart; } \
	~ClassName() { AtEnd; } \
};

#define __PROFILER_SCOPE_CLASS2(ClassName, TypeOne, One, TypeTwo, Two, AtStart, AtEnd) class ClassName \
{ \
public: \
	ClassName(TypeOne One, TypeTwo Two) { AtStart; } \
	~ClassName() { AtEnd; } \
};

	__PROFILER_SCOPE_CLASS1(Context, const char*, a_Name, Profiler::Get().PushContext(a_Name), Profiler::Get().PopContext());
	__PROFILER_SCOPE_CLASS2(ContextWithInfo, const char*, a_Name, const char*, a_Info, Profiler::Get().PushContext(a_Name, a_Info), Profiler::Get().PopContext());
	__PROFILER_SCOPE_CLASS2(Frame, const char*, a_Name, double, a_Double, char t_TimeString[32]; snprintf(t_TimeString, 32, "%3.4f", a_Double); Profiler::Get().PushContext(a_Name, t_TimeString);, Profiler::Get().PopContext());
	
	static Profiler::Logger Log;

	void PushContext(const char* a_Name, const char* a_Info = nullptr);
	void PopContext();

	const char* GetDataDump() const;
	size_t GetDataDumpSize() const;

	std::string GetJSON() const;
	bool IsCaptureFull() const { return m_BufferIterator >= BufferSize; }

	friend class ProfilerLogger;
protected:
	void WriteBuffer(char a_Type, const char * a_Message, const char* a_Message2 = nullptr);

	char m_Buffer[BufferSize];
	size_t m_BufferIterator = 0;
	size_t m_StackCount = 0;

private:
	Profiler() {}

	static Profiler* m_Instance;
};