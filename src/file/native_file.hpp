#pragma once
#if defined(_WIN32)
#include <file/base_file.hpp>

#include <memory>
#include <map>

class NativeFile : public BaseFile
{
public:
	using OnLoadFunction = std::function<void(NativeFile* a_File, LoadState)>;

	~NativeFile() {}

	bool Read(uint8_t* a_Destination, size_t a_ByteCount, size_t a_Offset = (size_t)-1) override;
	void Seek(size_t a_Offset, BaseFile::SeekType a_SeekFrom) override;

	virtual std::vector<uint8_t> Data() override;

	friend class NativeFileSystem;
protected:
	NativeFile(const std::string& a_File, const NativeFile::OnLoadFunction& a_OnLoadFunction = nullptr);

private:
	size_t m_ReadOffset = 0;

	std::vector<uint8_t> m_Data;
	OnLoadFunction m_OnLoadFunction;
};
#endif