#include <map>
#include <vector>
#include <fstream>
#include <cstdint>

using FileNameHash = uint64_t;
using FileMap = std::map<FileNameHash, std::vector<uint8_t>>;

class WAD
{
public:
	enum StorageType : uint8_t
	{
		UNCOMPRESSED = 0,
		ZLIB_COMPRESSED = 1,
		UNKNOWN = 2,
		ZSTD_COMPRESSED = 3
	};

	WAD(const char* a_FileName);

	bool HasFile(const char* a_FileName) const;
	bool ExtractFile(const char* a_FileName, const char* a_Destination);

private:
	void ParseHeader1();
	void ParseHeader3();

	struct FileData
	{
		uint32_t Offset;
		uint32_t CompressedSize;
		uint32_t FileSize;
		StorageType Type;
	};

	std::map<FileNameHash, FileData> m_FileData;
	std::ifstream t_FileStream;

	struct { char Major = 0, Minor = 0; } m_Version;
};