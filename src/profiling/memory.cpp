#include "memory.hpp"

#include <cstdio>
#include <string>

#if defined(NDEBUG) && !defined(DEBUG_MEMORY)

void Memory::Expose()
{
	printf("Memory::Expose was called but memory was not tracked!");
}

#else 

#include <map>

struct MemoryData
{
	std::string Name;
	std::string File;
	size_t Line;
};

std::map<void*, MemoryData>* m_Data = nullptr;

void Memory::Expose()
{
	if (m_Data == nullptr) return;

	for (auto& t_Data : *m_Data)
		printf("%s:%zu leaked a %s!\n", t_Data.second.File.c_str(), t_Data.second.Line, t_Data.second.Name.c_str());

	delete m_Data;
	m_Data = nullptr;
}

void Memory::DeletePointerInfo(const char * a_File, size_t a_Line, const char * a_Name, void * a_Pointer)
{
	if (m_Data == nullptr) return;

	const auto& t_Data = m_Data->find(a_Pointer);
	if (t_Data == m_Data->end()) return;

	m_Data->erase(t_Data);
}

void Memory::NewPointerInfo(const char * a_File, size_t a_Line, const char * a_Name, void * a_Pointer)
{
	if (!m_Data) m_Data = new std::map<void*, MemoryData>();

	auto& t_Element = (*m_Data)[a_Pointer];
	t_Element.File = a_File;
	t_Element.Line = a_Line;
	t_Element.Name = a_Name;
}

#endif
