#include "base_file.hpp"

BaseFile::BaseFile(const std::string & a_File) :
	m_Name(a_File)
{
}

bool BaseFile::IsLoaded() const
{
	return m_LoadState == LoadState::Loaded;
}

BaseFile::LoadState BaseFile::GetLoadState() const
{
	return m_LoadState;
}
