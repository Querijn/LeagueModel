#pragma once
#if defined(__EMSCRIPTEN__)

#include <file/base_file.hpp>

#include <memory>
#include <map>

class EmscriptenFile : public BaseFile
{
public:
	using OnLoadFunction = std::function<void(EmscriptenFile* a_File, LoadState)>;

	~EmscriptenFile() {}

	bool Read(uint8_t* a_Destination, size_t a_ByteCount, size_t a_Offset = (size_t)-1) override;
	void Seek(size_t a_Offset, BaseFile::SeekType a_SeekFrom) override;

	virtual std::vector<uint8_t> Data() override;

	friend class EmscriptenFileSystem;
protected:
	EmscriptenFile(const std::string& a_File, const EmscriptenFile::OnLoadFunction& a_OnLoadFunction = nullptr);

private:
	size_t m_ReadOffset = 0;

	std::vector<uint8_t> m_Data;
	OnLoadFunction m_OnLoadFunction;

	static std::map<std::string, EmscriptenFile*> m_LoadData;
	static void DataLoadSuccessHandler(const char* a_FileName);
	static void DataLoadFailHandler(const char* a_FileName);
};

#endif