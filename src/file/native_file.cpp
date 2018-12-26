#if defined(_WIN32)
#include "native_file.hpp"
#include "file_system.hpp"

#include <fstream>

void NativeFile::Load(NativeFile::OnLoadFunction a_OnLoadCallback, void* a_Argument)
{
	auto t_State = FetchData();
	if (a_OnLoadCallback) a_OnLoadCallback(this, t_State, a_Argument);
}

size_t NativeFile::Read(uint8_t* a_Destination, size_t a_ByteCount, size_t& a_Offset) const
{
	if (a_Offset + a_ByteCount > m_Size)
	{
		if (a_Offset > m_Size) return 0;
		else a_ByteCount = m_Size - a_Offset;
	}

	if (a_ByteCount == 0) return 0;

	memcpy(a_Destination, m_Data.data() + a_Offset, a_ByteCount);

	a_Offset += a_ByteCount;
	return a_ByteCount;
}

std::vector<uint8_t> NativeFile::GetData() const
{
	return m_Data;
}

std::string NativeFile::GetName() const
{
	return Name;
}

NativeFile::LoadState NativeFile::FetchData()
{
	auto t_FileName = Name.c_str();
	std::ifstream t_File(t_FileName, std::ios::binary | std::ios::ate);
	m_Size = t_File.tellg();
	if (m_Size == (size_t)-1)
	{
		m_Size = 0;
		return NativeFile::LoadState::NotFound;
	}

	t_File.seekg(0, std::ios::beg);
	m_Data.resize(m_Size);

	bool t_Read = !!t_File.read((char*)m_Data.data(), m_Size);
	t_File.close();

	return t_Read ? NativeFile::LoadState::Loaded : NativeFile::LoadState::FailedToLoad;
}
#endif