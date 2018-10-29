#include "image.hpp"

#include "file_system.hpp"

#include <stb_image.h>

#include <cstdint>
#include <iostream>

using DWORD = unsigned int;

#define DDSD_CAPS               0x00000001l
#define DDSD_HEIGHT             0x00000002l
#define DDSD_WIDTH              0x00000004l
#define DDSD_PITCH              0x00000008l
#define DDSD_PIXELFORMAT        0x00001000l
#define DDSD_LINEARSIZE         0x00080000l

#define DDPF_ALPHAPIXELS		0x00000001l
#define DDPF_FOURCC				0x00000004l
#define DDPF_RGB				0x00000040l

#pragma pack(push, 1)

struct DDPIXELFORMAT
{
	DWORD Size;
	DWORD Flags;
	DWORD FourCC;
	union
	{
		DWORD RGBBitCount;
		DWORD YUVBitCount;
		DWORD ZBufferBitDepth;
		DWORD AlphaBitDepth;
	};
	union
	{
		DWORD RBitMask;
		DWORD YBitMask;
	};
	union
	{
		DWORD GBitMask;
		DWORD UBitMask;
	};
	union {
		DWORD BBitMask;
		DWORD VBitMask;
	};
	union
	{
		DWORD RGBAlphaBitMask;
		DWORD YUVAlphaBitMask;
	};
};

struct DDSCAPS2
{
	DWORD Caps1;
	DWORD Caps2;
	DWORD Reserved[2];
};

struct DDSURFACEDESC2
{
	char Magic[4]; // "DDS "
	DWORD Size;
	DWORD Flags;
	DWORD Height;
	DWORD Width;
	DWORD PitchOrLinearSize;
	DWORD Depth;
	DWORD MipMapCount;
	DWORD Reserved1[11];
	DDPIXELFORMAT PixelFormat;
	DDSCAPS2 Caps;
	DWORD Reserved2;
};

#pragma pack(pop)

const uint8_t * Image::Data() const
{
	return m_Data.data();
}

Image::Type Image::GetType() const
{
	return m_Type;
}

Image::Image(const std::string & a_Path, const Image::OnLoadFunction & a_OnLoadFunction)
{
	
}
