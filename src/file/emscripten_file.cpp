#if defined(__EMSCRIPTEN__)
#include "emscripten_file.hpp"

#include <emscripten.h>

std::map<std::string, EmscriptenFile*> EmscriptenFile::m_LoadData;

bool EmscriptenFile::Read(uint8_t* a_Destination, size_t a_ByteCount, size_t a_Offset)
{
	if (a_Offset != (size_t)-1) return false; // TODO: Allow for reading from random locations

	return !!m_File->read(reinterpret_cast<char*>(a_Destination), a_ByteCount);
}

void EmscriptenFile::Seek(size_t a_Offset, SeekType a_SeekFrom)
{
	switch (a_SeekFrom)
	{
	case SeekType::FromBeginning:
		m_File->seekg(a_Offset, std::ios::beg);
		break;

	case SeekType::FromCurrent:
		m_File->seekg(a_Offset, std::ios::cur);
		break;

	case SeekType::FromEnd:
		m_File->seekg(a_Offset, std::ios::end);
		break;
	}
}

std::vector<uint8_t> EmscriptenFile::Data()
{
	auto t_Location = m_File->tellg();
	m_File->seekg(0, std::ios::beg);

	printf("EmscriptenFile::Data() for %zu bytes called, if large, this might be a slow process.\n", m_Size);
	std::vector<uint8_t> t_Data(m_Size);
	printf("EmscriptenFile::Data() has allocated %zu bytes.\n", m_Size);
	m_File->read(reinterpret_cast<char*>(t_Data.data()), m_Size);
	printf("EmscriptenFile::Data() has read %zu bytes of info, so we're done!\n", m_Size);

	m_File->seekg(t_Location, std::ios::beg);
	return t_Data;
}

EmscriptenFile::EmscriptenFile(const std::string & a_File, const EmscriptenFile::OnLoadFunction & a_OnLoadFunction) :
	BaseFile(a_File)
{
	m_OnLoadFunction = a_OnLoadFunction;

	printf("File added for \"%s\"\n", a_File.c_str());
	m_LoadData[a_File] = this;
	auto t_Index = m_LoadData.find(a_File);

	emscripten_async_wget(t_Index->first.c_str(), t_Index->first.c_str(), DataLoadSuccessHandler, DataLoadFailHandler);
}

void EmscriptenFile::DataLoadSuccessHandler(const char * a_FileName)
{
	std::string t_FileString = a_FileName;
	const char* t_FileName = t_FileString.c_str();
	while (*t_FileName == '/') t_FileName++; // Skip the annoying added / in front

	auto t_FileDataIndex = m_LoadData.find(t_FileName);
	if (t_FileDataIndex == m_LoadData.end())
	{
		printf("Unable to find corresponding file data: \"%s\"\n", t_FileName);
		/*printf("Unable to find corresponding file data for \"%s\".\nAvailable file data:\n", t_FileName);

		for (auto& t_Data : m_LoadData)
			printf("- %s\n", t_Data.first.c_str());*/
		return;
	}

	printf("Loading file: \"%s\"\n", t_FileDataIndex->first.c_str());
	auto t_FileData = t_FileDataIndex->second;
	t_FileData->m_File = std::make_shared<std::ifstream>(std::ifstream(a_FileName, std::ios::binary | std::ios::ate));
	t_FileData->m_Size = t_FileData->m_File->tellg();

	t_FileData->m_LoadState = BaseFile::LoadState::Loaded;
	if (t_FileData->m_OnLoadFunction)
	{
		printf("Calling external load function for \"%s\", file was succesfully loaded.\n", t_FileName);
		t_FileData->m_OnLoadFunction(t_FileData, t_FileData->m_LoadState);
	}
}

void EmscriptenFile::DataLoadFailHandler(const char * a_FileName)
{
	printf("DataLoadFailHandler: %s\n", a_FileName);

	auto& t_FileData = m_LoadData[a_FileName];
	t_FileData->m_LoadState = BaseFile::LoadState::FailedToLoad;
}

#endif