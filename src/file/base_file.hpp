#pragma once

#include <cstdint>
#include <vector>
#include <functional>
#include <string>

class BaseFile
{
public:
	enum SeekType
	{
		FromBeginning,
		FromEnd,
		FromCurrent
	};

	enum LoadState
	{
		NotLoaded,
		Loaded,
		FailedToLoad
	};

	BaseFile(const std::string& a_File);// , const BaseFile::OnLoadFunction& a_OnLoadFunction = nullptr);

	bool IsLoaded() const;
	LoadState GetLoadState() const;

	virtual bool Read(uint8_t* a_Destination, size_t a_ByteCount, size_t a_Offset = (size_t)-1) = 0;
	virtual void Seek(size_t a_Offset, SeekType a_SeekFrom) = 0;

	template<typename T>
	bool Get(T& a_DestinationObject, size_t a_Offset = (size_t)-1)
	{
		return Read((uint8_t*)&a_DestinationObject, sizeof(T), a_Offset);
	}

	template<typename T>
	bool Get(std::vector<T>& a_DestinationVector, size_t a_Count, size_t a_Offset = (size_t)-1)
	{
		a_DestinationVector.resize(a_Count);
		return Read(reinterpret_cast<uint8_t*>(a_DestinationVector.data()), a_Count * sizeof(T), a_Offset);
	}

	virtual std::vector<uint8_t> Data() = 0;
	std::string Name() const { return m_Name; }

protected:
	std::string m_Name;
	LoadState m_LoadState = LoadState::NotLoaded;
};