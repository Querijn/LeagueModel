#pragma once

#include <map>
#include <memory>
#include <string>

class StringView
{
public:
	StringView(const char* a_String);
	StringView(const std::string& a_String = "");

	std::string Get() const;

	size_t GetHash() const;

	StringView& operator=(const char* a_String);
	StringView& operator=(const std::string& a_String);

	bool operator==(const StringView& a_String) const;
	bool operator==(const std::string& a_String) const;
	bool operator==(const char* a_String) const;
	bool operator!=(const StringView& a_String) const;
	bool operator!=(const std::string& a_String) const;
	bool operator!=(const char* a_String) const;

private:
	static void GenerateRegister();
	size_t GenerateHash(const std::string& a_String) const;

	static std::map<size_t, std::string>* m_Register;
	size_t m_Hash;
};

struct StringViewCompare
{
	bool operator() (const StringView& lhs, const StringView& rhs) const;
};