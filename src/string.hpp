#pragma once

#include <map>
#include <vector>
#include <memory>

class String
{
public:
	String(const char* a_String);
	String(size_t a_Size = 0);
	String(size_t a_Size, char a_Fill);
	String(const String& a_String);
	~String();

	const char* Get() const;
	size_t Size() const;

	String& operator=(const char* a_String);

	String operator+(const char* a_String) const;
	String operator+(const String& a_String) const;

	String& operator+=(const char* a_String);
	String& operator+=(const String& a_String);

	bool operator==(const String& a_String) const;
	bool operator==(const char* a_String) const;
	bool operator!=(const String& a_String) const;
	bool operator!=(const char* a_String) const;

	char& operator[](size_t a_Index);

private:
	struct Space
	{
		size_t References = 0;
		size_t Capacity = 0;
	};

	static size_t GetAvailableOffset(size_t a_Size);
	static std::vector<char>* m_Pool;
	static std::map<size_t, Space>* m_Spaces;

	void SetContent(const char* a_String);
	void Alloc(size_t a_Size);
	
	friend class StringCompare;
protected:
	size_t m_Offset;

#if _DEBUG
	char* m_Data;
#endif
};

String operator+(const char *left, const String& right);

struct StringCompare
{
	bool operator() (const String& lhs, const String& rhs) const;
};

String ToString(uint32_t a_Value);