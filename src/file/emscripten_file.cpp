#if defined(__EMSCRIPTEN__)
#include "emscripten_file.hpp"

#include <emscripten.h>

#include <fstream>

std::map<std::string, EmscriptenFile*> EmscriptenFile::m_LoadData;

bool EmscriptenFile::Read(uint8_t* a_Destination, size_t a_ByteCount, size_t a_Offset)
{
	printf("Reading..\n");
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
	printf("Nulls: %s, %s\n", a_Destination == nullptr ? "true" : "false", t_Data == nullptr ? "true" : "false");

	for (int i = 0; i < a_ByteCount; i++)
		a_Destination[i] = t_Data[i];

	// If offset was undefined, update our read offset
	if (a_Offset == (size_t)-1) m_ReadOffset = t_Offset + a_ByteCount;

	return t_Result;
}

void EmscriptenFile::Seek(size_t a_Offset, SeekType a_SeekFrom)
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

std::vector<uint8_t> EmscriptenFile::Data()
{
	return m_Data;
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

	printf("Loading file: \"%s\"\n", t_FileName);

	auto& t_FileData = m_LoadData[t_FileName];

	std::ifstream t_File(a_FileName, std::ios::binary | std::ios::ate);
	std::streamsize t_Size = t_File.tellg();
	t_File.seekg(0, std::ios::beg);

	t_FileData->m_Data.resize(t_Size > 0 ? t_Size : 0);
	auto t_Read = !!t_File.read(reinterpret_cast<char*>(t_FileData->m_Data.data()), t_Size);
	t_FileData->m_LoadState = t_Read ? BaseFile::LoadState::Loaded : BaseFile::LoadState::FailedToLoad;
	auto t_PointerToFile = t_Read ? t_FileData : nullptr;

	if (t_FileData->m_OnLoadFunction)
	{
		printf("Calling external load function for \"%s\", file was %s loaded.\n", t_FileName, t_Read ? "succesfully" : "not");
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