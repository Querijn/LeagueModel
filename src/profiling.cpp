#include "profiling.hpp"

#include "platform.hpp"

#include <cassert>
#include <vector>
#include <algorithm>
#include <string>

Profiler* Profiler::m_Instance = nullptr;

Profiler& Profiler::Get()
{
	if (m_Instance == nullptr)
	{
		m_Instance = new Profiler;
		std::atexit([]()
		{
			delete m_Instance;
		});
	}

	return *m_Instance;
}

void Profiler::PushContext(const char * a_Name, const char* a_Info)
{
	WriteBuffer('B', a_Name, a_Info);
	m_StackCount++;
}

void Profiler::PopContext()
{
	WriteBuffer('E', nullptr);
	m_StackCount--;
}

void Profiler::WriteBuffer(char a_Type, const char * a_Message, const char* a_Message2)
{
	auto t_MessageSize = (a_Message ? strlen(a_Message) : 0);
	auto t_Message2Size = (a_Message2 ? strlen(a_Message2) : 0);
	auto t_ExpectedSize = sizeof(char) * 2 + t_MessageSize + t_Message2Size + sizeof(double);
	if (m_BufferIterator + t_ExpectedSize >= BufferSize)
	{
		m_BufferIterator = BufferSize;
		return;
	}

	m_Buffer[m_BufferIterator] = a_Type;
	m_BufferIterator += sizeof(char);

	if (a_Message != nullptr)
	{
		memcpy(&m_Buffer[m_BufferIterator], a_Message, strlen(a_Message));
		m_BufferIterator += strlen(a_Message);
	}

	if (a_Message2 != nullptr)
	{
		m_Buffer[m_BufferIterator] = ' ';
		m_BufferIterator += sizeof(char);

		memcpy(&m_Buffer[m_BufferIterator], a_Message2, strlen(a_Message2));
		m_BufferIterator += strlen(a_Message2);
	}

	m_Buffer[m_BufferIterator] = 0;
	m_BufferIterator += sizeof(char);

	auto t_Time = Platform::GetTimeSinceStart();
	memcpy(&m_Buffer[m_BufferIterator], &t_Time, sizeof(double));
	m_BufferIterator += sizeof(double);
}

std::string Profiler::GetJSON() const
{
	std::string t_Result = "[";

	static const std::string t_Start("{\"name\":\"");
	for (size_t i = 0; i < m_BufferIterator; )
	{
		char t_Type = m_Buffer[i];
		i++;

		const char* t_Name = &m_Buffer[i];
		i += strlen(t_Name) + 1;

		double t_Duration = *(double*)(&m_Buffer[i]);
		i += sizeof(double);

		t_Result += t_Start + t_Name + "\",\"ph\":\"" + t_Type + "\",\"pid\":1,\"ts\":" + std::to_string(t_Duration * 1e6) + "},";
	}

	return t_Result;
}

const char* Profiler::GetDataDump() const
{
	return m_Buffer;
}

size_t Profiler::GetDataDumpSize() const
{
	return m_BufferIterator;
}
