#include "managed_image.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define DDS_IMPLEMENTATION
#include <dds_reader.hpp>

namespace LeagueModel
{
	ManagedImage::ManagedImage(const char* path, OnLoadFunction onImageLoaded)
	{
		file = Spek::File::Load(path, [this, onImageLoaded](Spek::File::Handle f)
		{
			std::string fileName = f ? f->GetName() : "unknown";
			if (f == nullptr || f->GetLoadState() != Spek::File::LoadState::Loaded)
			{
				printf("Failed to load image %s.\n", fileName.c_str());
				loadState = Spek::File::LoadState::FailedToLoad;
				if (onImageLoaded)
					onImageLoaded(*this);
				return;
			}

			auto data = f->GetData();
			int imageWidth, imageHeight, bytesPerPixel;
			u8* imageData = nullptr;
			bool isDDS = DDS::is_dds(data);
			DDS::Image sourceImage;
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
				stbi_uc* imageData = stbi_load_from_memory(data.data(), data.size(), &imageWidth, &imageHeight, &bytesPerPixel, 4);
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
