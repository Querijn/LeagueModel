#include "managed_image.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define DDS_IMPLEMENTATION
#include <dds_reader.hpp>

namespace LeagueModel
{
#pragma pack(push, 1)
	struct TexHeader
	{
		char magic[4];
		u16 width;
		u16 height;
		u8 _pad1;
		u8 format;
		u8 _pad2;
		bool hasMipmaps;
	};
	static_assert(sizeof(TexHeader) == 12, "TexHeader is not packed correctly.");
#pragma pack(pop)

	ManagedImage::ManagedImage(const char* path, OnLoadFunction onImageLoaded)
	{
		file = Spek::File::Load(path, [this, onImageLoaded](Spek::File::Handle f)
		{
			std::string fileName = f ? f->GetName() : "unknown";
			if (f == nullptr || f->GetLoadState() != Spek::File::LoadState::Loaded)
			{
				printf("Failed to load image %s.\n", fileName.c_str());
				loadState = Spek::File::LoadState::FailedToLoad;

				// TODO: Do something better than create a 1x1 black image.
				const u8 imageData[4] = { 0, 0, 0, 255 };
				sg_image_desc imageDesc = {};
				imageDesc.width = 1;
				imageDesc.height = 1;
				imageDesc.pixel_format = SG_PIXELFORMAT_RGBA8;
				imageDesc.data.subimage[0][0].ptr = imageData;
				imageDesc.data.subimage[0][0].size = 4;
				image = sg_make_image(imageDesc);

				if (onImageLoaded)
					onImageLoaded(*this);
				return;
			}

			int imageWidth, imageHeight, bytesPerPixel;
			u8* imageData = nullptr;
			bool isDDS = DDS::is_dds(f->GetData());
			DDS::Image sourceImage;

			std::vector<u8> data = f->GetData();
			bool isTex = !isDDS && data.size() > 4 && memcmp(data.data(), "TEX\0", 4) == 0;
			if (isTex)
			{
				size_t offset = 0;
				TexHeader header;
				f->Get(header, offset);

				DDS::DDSHEADER ddsHeader = {};
				ddsHeader.dwMagic = MAKEFOURCC('D', 'D', 'S', ' ');
				ddsHeader.surfaceDesc.dwSize = 124;
				ddsHeader.surfaceDesc.dwFlags = DDS::DDSD_CAPS | DDS::DDSD_HEIGHT | DDS::DDSD_WITH | DDS::DDSD_PIXELFORMAT;
				ddsHeader.surfaceDesc.ddpfPixelFormat.dwSize = 32;
				ddsHeader.surfaceDesc.dwWidth = header.width;
				ddsHeader.surfaceDesc.dwHeight = header.height;
				ddsHeader.surfaceDesc.dwPitchOrLinearSize = header.width * header.height;
				ddsHeader.surfaceDesc.ddsCaps.dwCaps1 = DDS::DDSCAPS_TEXTURE;

				if (header.format == 0x0a)
				{
					ddsHeader.surfaceDesc.ddpfPixelFormat.dwFourCC = FOURCC_DXT1;
					ddsHeader.surfaceDesc.ddpfPixelFormat.dwFlags = DDS::DDPF_FOURCC;
				}
				else if (header.format == 0x0c)
				{
					ddsHeader.surfaceDesc.ddpfPixelFormat.dwFourCC = FOURCC_DXT5;
					ddsHeader.surfaceDesc.ddpfPixelFormat.dwFlags = DDS::DDPF_FOURCC;
				}
				else if (header.format == 0x14) // BGRA8
				{
					ddsHeader.surfaceDesc.ddpfPixelFormat.dwFlags = DDS::DDPF_ALPHAPIXELS | DDS::DDPF_RGB;
					ddsHeader.surfaceDesc.ddpfPixelFormat.dwRGBBitCount = 32;
					ddsHeader.surfaceDesc.ddpfPixelFormat.dwRBitMask = 0x000000ff;
					ddsHeader.surfaceDesc.ddpfPixelFormat.dwGBitMask = 0x0000ff00;
					ddsHeader.surfaceDesc.ddpfPixelFormat.dwBBitMask = 0x00ff0000;
					ddsHeader.surfaceDesc.ddpfPixelFormat.dwRGBAlphaBitMask = 0xff000000;
				}
				else
				{
					printf("Unknown TEX format: %X\n", header.format);
					loadState = Spek::File::LoadState::FailedToLoad;
					if (onImageLoaded)
						onImageLoaded(*this);
					return;
				}

				u8* pixels = nullptr;
				size_t pixelCount = 0;
				if (header.hasMipmaps)
				{
					int blockSize = 0;
					int bytesPerBlock = 0;
					if (header.format == 0x0a)
					{
						blockSize = 4;
						bytesPerBlock = 8;
					}
					else if (header.format == 0x0c)
					{
						blockSize = 4;
						bytesPerBlock = 16;
					}
					else if (header.format == 0x14)
					{
						blockSize = 1;
						bytesPerBlock = 4;
					}

					int powerOf2 = std::max(header.width, header.height);
					int mipmapCount = 0;
					while (powerOf2 > 1)
					{
						mipmapCount += 1;
						powerOf2 >>= 1;
					}

					int blockWidth = (header.width + blockSize - 1) / blockSize;
					int blockHeight = (header.height + blockSize - 1) / blockSize;
					int mipmapSize = bytesPerBlock * blockWidth * blockHeight;

					pixels = data.data() + data.size() - mipmapSize;
					pixelCount = mipmapSize;
				}
				else
				{
					pixels = data.data() + sizeof(TexHeader);
					pixelCount = header.width * header.height;
				}

				// Turn data into a DDS file, and then load it as one.
				data.resize(sizeof(DDS::DDSHEADER) + pixelCount);
				memcpy(data.data(), &ddsHeader, sizeof(DDS::DDSHEADER));
				memcpy(data.data() + sizeof(DDS::DDSHEADER), pixels, pixelCount);
				isDDS = true;
			}

			if (isDDS)
			{
				sourceImage = DDS::read_dds(data);
				imageWidth = sourceImage.width;
				imageHeight = sourceImage.height;
				bytesPerPixel = sourceImage.bpp;
				imageData = sourceImage.data.data();
			}
			else
			{
				imageData = stbi_load_from_memory(data.data(), data.size(), &imageWidth, &imageHeight, &bytesPerPixel, 4);
			}

			if (imageData == nullptr)
			{
				loadState = Spek::File::LoadState::FailedToLoad;
				if (onImageLoaded)
					onImageLoaded(*this);
				return;
			}

			if (image.id != SG_INVALID_ID)
			{
				sg_destroy_image(image);
				image.id = SG_INVALID_ID;
			}

			sg_image_desc imageDesc = {};
			imageDesc.width = imageWidth;
			imageDesc.height = imageHeight;
			imageDesc.pixel_format = bytesPerPixel == 4 ? SG_PIXELFORMAT_RGBA8 : SG_PIXELFORMAT_R8;
			imageDesc.data.subimage[0][0].ptr = imageData;
			imageDesc.data.subimage[0][0].size = (size_t)imageWidth * (size_t)imageHeight * bytesPerPixel;
			image = sg_make_image(imageDesc);

			if (isDDS == false)
				stbi_image_free(imageData);
			loadState = Spek::File::LoadState::Loaded;
			if (onImageLoaded)
				onImageLoaded(*this);
		});
	}

	ManagedImage::~ManagedImage()
	{
		if (image.id != SG_INVALID_ID)
		{
			sg_destroy_image(image);
			image.id = SG_INVALID_ID;
		}
	}
}
