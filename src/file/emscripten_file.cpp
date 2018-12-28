#if defined(__EMSCRIPTEN__)
#include "league_model/file/emscripten_file.hpp"

#include <emscripten.h>

void EmscriptenFile::Load(EmscriptenFile::OnLoadFunction a_OnLoadCallback, void* a_Argument)
{
	m_Handled = false;
	if (m_State != FileLoadState::NotLoaded)
	{
		if (a_OnLoadCallback) 
			a_OnLoadCallback(this, m_State, a_Argument);
		return;
	}

	m_OnLoadArg[m_ArgCount] = a_OnLoadCallback;
	m_ArgData[m_ArgCount] = a_Argument;
	m_ArgCount++;

	if (m_ArgCount == 1) // Only load on the first request
		emscripten_async_wget_data(Name.c_str(), (void*)this, OnLoad, OnLoadFailed);
}

EmscriptenFile::EmscriptenFile(const std::string & a_FileName) : 
	BaseFile(a_FileName), m_ArgCount(0)
{
	for (int i = 0; i < MaxCallbacks; i++)
	{
		m_OnLoadArg[i] = nullptr;
		m_ArgData[i] = nullptr;
	}
}

void EmscriptenFile::OnLoad(void* a_Argument, void* a_Data, int a_Size)
{
	auto* t_File = (EmscriptenFile*)a_Argument;
	t_File->m_Data.resize(a_Size);
	t_File->m_Size = a_Size;

	memcpy(t_File->m_Data.data(), a_Data, a_Size);

	t_File->m_State = EmscriptenFile::LoadState::Loaded;
	
	for (int i = 0; i < t_File->m_ArgCount; i++)
	{
		if (t_File->m_OnLoadArg[i])
			t_File->m_OnLoadArg[i](t_File, EmscriptenFile::LoadState::Loaded, t_File->m_ArgData[i]);
	}
	t_File->m_Handled = true;
}

void EmscriptenFile::OnLoadFailed(void* a_Argument)
{
	auto* t_File = (EmscriptenFile*)a_Argument;
	t_File->m_Size = 0;

	t_File->m_State = EmscriptenFile::LoadState::NotFound;
	for (int i = 0; i < t_File->m_ArgCount; i++)
		if (t_File->m_OnLoadArg[i]) t_File->m_OnLoadArg[i](t_File, EmscriptenFile::LoadState::NotFound, t_File->m_ArgData[i]);
	t_File->m_Handled = true;
}

size_t EmscriptenFile::Read(uint8_t* a_Destination, size_t a_ByteCount, size_t& a_Offset) const
{
	if (a_Offset + a_ByteCount > m_Size)
	{
		printf("OUT OF BUFFER RANGE FOR %s: We're returning a reduced size => %lu\n", Name.c_str(), a_Offset > m_Size ? 0 : m_Size - a_Offset);
		if (a_Offset > m_Size) return 0;
		else a_ByteCount = m_Size - a_Offset;
	}

	if (a_ByteCount == 0) return 0;

	memcpy(a_Destination, m_Data.data() + a_Offset, a_ByteCount);

	a_Offset += a_ByteCount;
	return a_ByteCount;
}

const std::vector<uint8_t>& EmscriptenFile::GetData() const
{
	return m_Data;
}

std::string EmscriptenFile::GetName() const
{
	return Name;
}

#endif