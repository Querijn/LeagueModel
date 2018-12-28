#pragma once
#if defined(__EMSCRIPTEN__)

#include <league_model/file/base_file.hpp>

#include <vector>

class EmscriptenFile : public BaseFile
{
	static const int MaxCallbacks = 32;

public:
	using LoadState = FileLoadState;
	using OnLoadFunction = void(*)(EmscriptenFile* a_File, FileLoadState a_LoadState, void* a_Argument);

	void Load(OnLoadFunction a_OnLoadCallback, void* a_Argument = nullptr);
	size_t Read(uint8_t* a_Destination, size_t a_ByteCount, size_t& a_Offset) const;

	template<typename T>
	bool Get(T& a_Element, size_t& a_Offset)
	{
		bool t_Read = Read((uint8_t*)&a_Element, sizeof(T), a_Offset) == sizeof(T);
		return t_Read;
	}

	template<typename T>
	bool Get(std::vector<T>& a_Element, size_t& a_Offset)
	{
		return false;
	}

	template<typename T>
	bool Get(std::vector<T>& a_Element, size_t a_Count, size_t& a_Offset)
	{
		a_Element.clear();

		bool t_Worked = true;
		for (int i = 0; i < a_Count; i++)
		{
			T t_Element;
			if (Get(t_Element, a_Offset) != sizeof(T))
				t_Worked = false;

			auto t_Name = std::to_string(t_Element);
			a_Element.push_back(t_Element);
		}

		return t_Worked;
	}

	// This function doesn't always work due to alignment. Don't use for anything that isn't POD.
	template<typename T>
	bool GetBuffer(std::vector<T>& a_Element, size_t a_Count, size_t& a_Offset)
	{
		a_Element.resize(a_Count);
		return Read((uint8_t*)a_Element.data(), a_Count * sizeof(T), a_Offset) == a_Count * sizeof(T);
	}

	const std::vector<uint8_t>& GetData() const;
	std::string GetName() const;

	friend class FileSystem;
protected:
	EmscriptenFile(const std::string& a_FileName);

private:
	static void OnLoad(void* a_Argument, void* a_Data, int a_Size);
	static void OnLoadFailed(void* a_Argument);

	std::vector<uint8_t> m_Data;
	std::shared_ptr<std::ifstream> m_Stream;
	
	OnLoadFunction m_OnLoadArg[MaxCallbacks];
	void* m_ArgData[MaxCallbacks];
	size_t m_ArgCount = 0;
};

#endif