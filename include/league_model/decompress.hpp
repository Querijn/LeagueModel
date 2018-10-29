#pragma once

#include <cstdint>
#include <map>
#include <vector>
#include <string>

using FileNameHash = size_t;
using FileMap = std::map<FileNameHash, std::vector<uint8_t>>;

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
		BaseWAD Begin;

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
		BaseWAD base;
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
		uint8_t Type;
		uint8_t Duplicate;
		uint8_t Unknown[2];
		uint64_t SHA256;
	};
}
#pragma pack(pop)

FileMap DecompressWAD(const char* a_FileName);
FileMap DecompressCompressedFile(const char* a_FileName);