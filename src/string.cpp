#include "string.hpp"

#include <cassert>

static const size_t g_SpaceSizeIncrement = 1024;
std::vector<char>* String::m_Pool = nullptr;
std::map<size_t, String::Space>* String::m_Spaces = nullptr;

String::String(const char * a_String)
{
	SetContent(a_String);
}

String::String(size_t a_Size)
{
	Alloc(a_Size);
}

String::String(size_t a_Size, char a_Fill)
{
	Alloc(a_Size);
	memset(&(*m_Pool)[m_Offset], a_Fill, a_Size);
}

String::String(const String & a_String) :
	m_Offset(a_String.m_Offset)

#if _DEBUG
	, m_Data(a_String.m_Data)
#endif
{
	(*m_Spaces)[a_String.m_Offset].References++;
}

String::~String()
{
	(*m_Spaces)[m_Offset].References--;
}

const char* String::Get() const
{
	return &(*m_Pool)[m_Offset];
}

size_t String::Size() const
{
	return strlen(Get());
}

String& String::operator=(const char* a_String)
{
	SetContent(a_String);
	return *this;
}

String String::operator+(const char * a_String) const
{
	String t_Combination(strlen(Get()) + strlen(a_String));
	char* t_Offset = &(*m_Pool)[t_Combination.m_Offset];
	strcpy(t_Offset, Get());
	strcat(t_Offset, a_String);

	return t_Combination;
}

String String::operator+(const String& a_String) const
{
	String t_Combination(strlen(Get()) + strlen(a_String.Get()));
	char* t_Offset = &(*m_Pool)[t_Combination.m_Offset];
	strcpy(t_Offset, Get());
	strcat(t_Offset, a_String.Get());

	return t_Combination;
}

String & String::operator+=(const char * a_String)
{
	String t_Combination(strlen(Get()) + strlen(a_String));
	char* t_Offset = &(*m_Pool)[t_Combination.m_Offset];
	strcpy(t_Offset, Get());
	strcat(t_Offset, a_String);

	(*m_Spaces)[m_Offset].References--;
	m_Offset = t_Combination.m_Offset;
	(*m_Spaces)[m_Offset].References++;

	return *this;
}

String & String::operator+=(const String& a_String)
{
	String t_Combination(strlen(Get()) + strlen(a_String.Get()));
	char* t_Offset = &(*m_Pool)[t_Combination.m_Offset];
	strcpy(t_Offset, Get());
	strcat(t_Offset, a_String.Get());

	(*m_Spaces)[m_Offset].References--;
	m_Offset = t_Combination.m_Offset;
	(*m_Spaces)[m_Offset].References++;

	return *this;
}

bool String::operator==(const String& a_String) const
{
	if (m_Offset == a_String.m_Offset)
		return true;

	return strcmp(Get(), a_String.Get()) == 0;
}

bool String::operator==(const char * a_String) const
{
	return strcmp(Get(), a_String) == 0;
}

bool String::operator!=(const String& a_String) const
{
	return !operator==(a_String);
}

bool String::operator!=(const char * a_String) const
{
	return !operator==(a_String);
}

char & String::operator[](size_t a_Index)
{
	return (*m_Pool)[m_Offset + a_Index];
}

size_t String::GetAvailableOffset(size_t a_Size)
{
	for (auto& t_KeyValue : *m_Spaces)
	{
		auto t_Offset = t_KeyValue.first;
		auto& t_Space = t_KeyValue.second;
		if (t_Space.References == 0 && t_Space.Capacity >= a_Size)
		{
			(*m_Spaces)[t_Offset + a_Size].Capacity = t_Space.Capacity - a_Size;

			t_Space.Capacity = a_Size;
			return t_KeyValue.first;
		}
	}

	assert(a_Size < g_SpaceSizeIncrement);

	size_t t_Offset = m_Pool->size();
	m_Pool->resize(t_Offset + g_SpaceSizeIncrement);
	(*m_Spaces)[t_Offset].Capacity = a_Size;
	(*m_Spaces)[t_Offset + a_Size].Capacity = g_SpaceSizeIncrement - a_Size;
	return t_Offset;
}

void String::SetContent(const char * a_String)
{
	Alloc(strlen(a_String));

	strcpy(&(*m_Pool)[m_Offset], a_String);
}

void String::Alloc(size_t a_Size)
{
	if (m_Pool == nullptr)
	{
		m_Pool = new std::vector<char>(g_SpaceSizeIncrement);
		m_Spaces = new std::map<size_t, Space>();

		(*m_Spaces)[0].Capacity = g_SpaceSizeIncrement;
	}

	m_Offset = GetAvailableOffset(a_Size + 1);
	(*m_Spaces)[m_Offset].References++;
	(*m_Pool)[m_Offset] = '\0';

#if _DEBUG
	m_Data = &(*m_Pool)[m_Offset];
#endif
}

bool StringCompare::operator()(const String & lhs, const String & rhs) const
{
	return lhs.m_Offset < rhs.m_Offset;
}

String operator+(const char * left, const String& right)
{
	String t_Left(left);
	return t_Left + right;
}

String ToString(uint32_t a_Value)
{
	char t_Buffer[32];
	snprintf(t_Buffer, 32, "%d", a_Value);

	return t_Buffer;
}