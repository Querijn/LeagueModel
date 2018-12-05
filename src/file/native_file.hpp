#pragma once
#if defined(_WIN32)

#include <file/base_file.hpp>
#include <vector>

class NativeFile : public BaseFile
{
public:
	using LoadState = FileLoadState;
	using OnLoadFunction = void(*)(NativeFile* a_File, FileLoadState a_LoadState, void* a_Argument);

	void Load(OnLoadFunction a_OnLoadCallback, void* a_Argument = nullptr);
	size_t Read(uint8_t* a_Destination, size_t a_ByteCount, size_t& a_Offset) const;

	std::vector<uint8_t> GetData() const;
	std::string GetName() const;

	template<typename T>
	bool Get(T& a_Element, size_t& a_Offset)
	{
		return Read((uint8_t*)&a_Element, sizeof(T), a_Offset) == sizeof(T);
	}

	template<typename T>
	bool Get(std::vector<T>& a_Element, size_t a_Offset)
	{
		static_assert(false, "Can't use Get(a_Element, a_Offset) with an std::vector");
		return false;
	}

	template<typename T>
	bool Get(std::vector<T>& a_Element, size_t a_Count, size_t& a_Offset)
	{
		a_Element.resize(a_Count);
		return Read((uint8_t*)a_Element.data(), a_Count * sizeof(T), a_Offset) == a_Count * sizeof(T);
	}

	// Same as above but this one forces the aligned read for platforms that don't support x86-esque alignment
	template<typename T>
	bool GetBuffer(std::vector<T>& a_Element, size_t a_Count, size_t& a_Offset)
	{
		a_Element.resize(a_Count);
		return Read((uint8_t*)a_Element.data(), a_Count * sizeof(T), a_Offset) == a_Count * sizeof(T);
	}

	friend class FileSystem;
protected:
	NativeFile(std::string a_FileName) : BaseFile(a_FileName) {}

private:
	std::vector<uint8_t> m_Data;

	FileLoadState FetchData();
};

#endif