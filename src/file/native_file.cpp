#if defined(_WIN32)
#include "native_file.hpp"

#include <fstream>

bool NativeFile::Read(uint8_t * a_Destination, size_t a_ByteCount, size_t a_Offset)
{
	// If offset was undefined, use our internal read offset
	size_t t_Offset = a_Offset == (size_t)-1 ? m_ReadOffset : a_Offset;

	// Make sure our offset is 
	bool t_Result = true;
	if (t_Offset + a_ByteCount >= m_Data.size())
	{
		a_ByteCount = m_Data.size() - t_Offset;
		t_Result = false;
	}
	else if (t_Offset >= m_Data.size())
	{
		a_ByteCount = 0;
		t_Result = false;
	}

	uint8_t* t_Data = m_Data.data() + t_Offset;
	memcpy(a_Destination, t_Data, a_ByteCount);

	// If offset was undefined, update our read offset
	if (a_Offset == (size_t)-1) m_ReadOffset = t_Offset + a_ByteCount;

	return t_Result;
}

void NativeFile::Seek(size_t a_Offset, SeekType a_SeekFrom)
{
	switch (a_SeekFrom)
	{
	case SeekType::FromBeginning:
		m_ReadOffset = a_Offset;
		break;

	case SeekType::FromCurrent:
		m_ReadOffset += a_Offset;
		break;

	case SeekType::FromEnd:
		m_ReadOffset = m_Data.size() - a_Offset;
		break;
	}
}

std::vector<uint8_t> NativeFile::Data()
{
	return m_Data;
}

NativeFile::NativeFile(const std::string & a_File, const NativeFile::OnLoadFunction & a_OnLoadFunction) :
	BaseFile(a_File)
{
	std::ifstream t_File(a_File, std::ios::binary | std::ios::ate);
	std::streamsize t_Size = t_File.tellg();
	t_File.seekg(0, std::ios::beg);

	m_Data.resize(t_Size > 0 ? t_Size : 0);
	auto t_Read = !!t_File.read(reinterpret_cast<char*>(m_Data.data()), t_Size);
	m_LoadState = t_Read ? BaseFile::LoadState::Loaded : BaseFile::LoadState::FailedToLoad;

	if (a_OnLoadFunction) a_OnLoadFunction(this, m_LoadState);
}
#endif