#if defined(__EMSCRIPTEN__)
#include "emscripten_file.hpp"

#include <emscripten.h>

void EmscriptenFile::Load(EmscriptenFile::OnLoadFunction a_OnLoadCallback, void* a_Argument)
{
	m_OnLoadArg = a_OnLoadCallback;
	//m_OnLoad = nullptr;
	m_ArgData = a_Argument;

	auto t_Name = m_Name.Get();
	emscripten_async_wget_data(t_Name.c_str(), (void*)this, OnLoad, OnLoadFailed);
}

void EmscriptenFile::OnLoad(void* a_Argument, void* a_Data, int a_Size)
{
	auto* t_File = (EmscriptenFile*)a_Argument;
	auto t_FileName = t_File->GetName().Get();

	t_File->m_Data.resize(a_Size);
	t_File->m_Size = a_Size;

	memcpy(t_File->m_Data.data(), a_Data, a_Size);

	if (t_File->m_OnLoadArg) t_File->m_OnLoadArg(t_File, EmscriptenFile::LoadState::Loaded, t_File->m_ArgData);
}

void EmscriptenFile::OnLoadFailed(void* a_Argument)
{
	auto* t_File = (EmscriptenFile*)a_Argument;
	auto t_FileName = t_File->GetName().Get();

	t_File->m_Size = 0;
	if (t_File->m_OnLoadArg) t_File->m_OnLoadArg(t_File, EmscriptenFile::LoadState::NotFound, t_File->m_ArgData);
}

size_t EmscriptenFile::Read(uint8_t* a_Destination, size_t a_ByteCount, size_t& a_Offset) const
{
	if (a_Offset + a_ByteCount > m_Size) a_ByteCount = m_Size - a_Offset;
	if (a_ByteCount == 0) return 0;

	memcpy(a_Destination, &m_Data[a_Offset], a_ByteCount);

	a_Offset += a_ByteCount;
	return a_ByteCount;
}

const std::vector<uint8_t>& EmscriptenFile::GetData() const
{
	return m_Data;
}

StringView EmscriptenFile::GetName() const
{
	return m_Name;
}

#endif