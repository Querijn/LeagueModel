#include <wad_file.hpp>
#include <cassert>
#include <xxhash64.h>
#if defined(_WIN32)
#include <direct.h>
#endif

extern "C"
{
	#define ZSTD_STATIC_LINKING_ONLY
	#include <zlib.h>
	#include <zstd.h>
}

#pragma pack(push, 1)
struct BaseWAD
{
	char Magic[2]; // RW
	char Major;
	char Minor;

	bool IsValid() const { return Magic[0] == 'R' && Magic[1] == 'W'; }
};

namespace WADv1
{
	struct Header
	{
		BaseWAD Base;
		uint16_t Offset;
		uint16_t CellSize;
		uint32_t FileCount;
	};

	struct FileHeader
	{
		uint64_t PathHash;
		uint32_t Offset;
		uint32_t CompressedSize;
		uint32_t FileSize;
		uint32_t Type;
	};
}

namespace WADv3
{
	struct Header
	{
		BaseWAD Base;
		char ECDSA[256];
		uint64_t Checksum;
		uint32_t FileCount;
	};

	struct FileHeader
	{
		uint64_t PathHash;
		uint32_t Offset;
		uint32_t CompressedSize;
		uint32_t FileSize;
		WAD::StorageType Type;
		uint8_t Duplicate;
		uint8_t Unknown[2];
		uint64_t SHA256;
	};
}
#pragma pack(pop)

WAD::WAD(const char * a_FileName)
{
	t_FileStream.open(a_FileName, std::ifstream::binary);
	if (!t_FileStream) return;

	BaseWAD t_WAD;
	t_FileStream.read(reinterpret_cast<char*>(&t_WAD), sizeof(BaseWAD));
	assert(t_WAD.IsValid(), "The WAD header is not valid!");

	m_Version.Major = t_WAD.Major;
	m_Version.Minor = t_WAD.Minor;

	t_FileStream.seekg(0, std::ios::beg);

	/*if (m_Version.Major == 1)
		ParseHeader1();
	else */if (m_Version.Major == 3)
		ParseHeader3();
	else assert(false, "Unexpected major version!");
}

bool WAD::HasFile(const char * a_FileName) const
{
	uint64_t t_Hash = XXHash64::hash(a_FileName, strlen(a_FileName), 0);
	return m_FileData.find(t_Hash) != m_FileData.end();
}

void CreateRequiredFolders(const std::string& a_Path)
{
	for (size_t i = 0; i < a_Path.length();)
	{
		auto t_FolderIndex = a_Path.find('/', i);
		if (t_FolderIndex == std::string::npos) break;
		auto t_Folder = std::string(&a_Path[0], &a_Path[t_FolderIndex]);

		struct stat t_Buffer;
		if (stat(t_Folder.c_str(), &t_Buffer) != 0)
		{
#if defined(_WIN32)
			_mkdir(t_Folder.c_str());
#else 
			mkdir(t_Folder.c_str(), 0733);
#endif
		}

		i = t_FolderIndex + 1;
	}
}

bool WAD::ExtractFile(const char * a_FileName, const char* a_Destination)
{
	char t_FileName[FILENAME_MAX];
	strncpy(t_FileName, a_FileName, FILENAME_MAX);

	for (char* t_Char = t_FileName; *t_Char; t_Char++) 
		*t_Char = tolower(*t_Char);

	uint64_t t_Hash = XXHash64::hash(t_FileName, strlen(t_FileName), 0);
	const auto& t_FileDataIterator = m_FileData.find(t_Hash);
	if (t_FileDataIterator == m_FileData.end())
		return false;

	strncpy(t_FileName, a_Destination, FILENAME_MAX);
	for (char* t_Char = t_FileName; *t_Char; t_Char++)
		*t_Char = tolower(*t_Char);

	const auto& t_FileData = t_FileDataIterator->second;
	std::vector<uint8_t> t_Result;

	t_FileStream.seekg(t_FileData.Offset, t_FileStream.beg);

	switch (t_FileData.Type)
	{
	case WAD::StorageType::UNCOMPRESSED:
	{
		t_Result.resize(t_FileData.FileSize);
		t_FileStream.read((char*)t_Result.data(), t_FileData.FileSize);
		break;
	}

	case WAD::StorageType::UNKNOWN:
		printf("Unknown WAD storage type found\n");
		__debugbreak();
		break;

	case WAD::StorageType::ZLIB_COMPRESSED:
	{
		throw 0;
		// Not implemented

		/*int t_InflateResult;
		unsigned t_BytesReceived;
		z_stream t_Stream;
		static const size_t t_Chunk = 1024;
		size_t t_Written = 0;

		t_Stream.zalloc = Z_NULL;
		t_Stream.zfree = Z_NULL;
		t_Stream.opaque = Z_NULL;
		t_Stream.avail_in = t_FileData.CompressedSize;
		t_Stream.next_in = (uint8_t*)(m_Data.get() + t_FileData.Offset);
		t_InflateResult = inflateInit(&t_Stream);

		do
		{
			t_Result.resize(t_Written + t_Chunk);

			t_Stream.avail_out = t_Chunk;
			t_Stream.next_out = t_Result.data() + t_Written;
			t_InflateResult = inflate(&t_Stream, Z_NO_FLUSH);

			switch (t_InflateResult)
			{
			case Z_NEED_DICT:
				t_InflateResult = Z_DATA_ERROR;

			case Z_DATA_ERROR:
			case Z_STREAM_ERROR:
			case Z_MEM_ERROR:
				(void)inflateEnd(&t_Stream);
				printf("Error occurred during ZLIB inflation: %d\n", t_InflateResult);
				break;
			}

			t_BytesReceived = t_Chunk - t_Stream.avail_out;
			t_Written += t_BytesReceived;
			t_Result.resize(t_Written);

		} while (t_Stream.avail_out == 0);
		break;*/
	}


	case WAD::StorageType::ZSTD_COMPRESSED:
	{
		std::unique_ptr<char[]> t_CompressedData(new char[t_FileData.CompressedSize]);
		t_FileStream.read(t_CompressedData.get(), t_FileData.CompressedSize);

		unsigned long long const t_UncompressedSize = t_FileData.FileSize; // ZSTD_findDecompressedSize(t_CompressedData.get(), t_FileData.CompressedSize);

		/*if (t_UncompressedSize == ZSTD_CONTENTSIZE_ERROR || t_UncompressedSize == ZSTD_CONTENTSIZE_UNKNOWN)
		{
			printf("Error occurred during ZSTD decompressing: Could not indentify content size. (%s)\n", t_UncompressedSize == ZSTD_CONTENTSIZE_ERROR ? "ZSTD_CONTENTSIZE_ERROR" : "ZSTD_CONTENTSIZE_UNKNOWN");
			break;
		}*/

		std::vector<uint8_t> t_Uncompressed;
		t_Uncompressed.resize(t_UncompressedSize);
		size_t const t_Size = ZSTD_decompress(t_Uncompressed.data(), t_UncompressedSize, t_CompressedData.get(), t_FileData.CompressedSize);

		t_Result = t_Uncompressed;
		break;
	}

	default:
		printf("Unidentified storage type detected: %d\n", t_FileData.Type);
		break;
	}

	if (t_Result.size() == 0)
		return false;

	CreateRequiredFolders(t_FileName);
	std::ofstream t_OutputFile;
	t_OutputFile.open(t_FileName, std::ios::binary | std::ios::out);
	t_OutputFile.write((const char*)t_Result.data(), t_Result.size());
	t_OutputFile.close();
	return true;
}

void WAD::ParseHeader3()
{
	WADv3::Header t_WAD;
	t_FileStream.read(reinterpret_cast<char*>(&t_WAD), sizeof(WADv3::Header));

	for (int i = 0; i < t_WAD.FileCount; i++)
	{
		WADv3::FileHeader t_Source;
		t_FileStream.read(reinterpret_cast<char*>(&t_Source), sizeof(WADv3::FileHeader));
		WAD::FileData t_Dest;
		
		t_Dest.Offset = t_Source.Offset;
		t_Dest.CompressedSize = t_Source.CompressedSize;
		t_Dest.FileSize = t_Source.FileSize;
		t_Dest.Type = t_Source.Type;

		m_FileData[t_Source.PathHash] = t_Dest;
	}
}