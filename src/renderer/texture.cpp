#include <renderer/opengl.hpp>
#include <renderer/texture.hpp>

#include <dds.h>

#include <algorithm>

Texture::Texture()
{
}

void Texture::Load(const std::string & a_ImagePath, Texture::OnLoadFunction a_OnLoadFunction)
{
	FileSystem t_FileSystem;

	t_FileSystem.OpenFile(a_ImagePath, [&](BaseFile* a_File, BaseFile::LoadState a_LoadState)
	{
		if (a_LoadState != BaseFile::LoadState::Loaded)
		{
			if (a_OnLoadFunction) a_OnLoadFunction(nullptr, a_LoadState);
			return;
		}

		// Check if image path ends in DDS
		auto t_Index = a_ImagePath.find_last_of('.');
		auto t_IsDDS = (a_ImagePath[t_Index + 1] == 'd' || a_ImagePath[t_Index + 1] == 'D') && (a_ImagePath[t_Index + 2] == 'd' || a_ImagePath[t_Index + 2] == 'D') && (a_ImagePath[t_Index + 3] == 's' || a_ImagePath[t_Index + 3] == 'S');

		if (t_IsDDS) UploadDDS(a_File, a_OnLoadFunction);
		else UploadRGB(a_File, a_OnLoadFunction);
	});
	
}

Texture::~Texture()
{
}

glm::vec2 Texture::GetDimensions() const
{
	return m_Dimensions;
}

Texture::operator int() const
{
	return m_ID;
}

void Texture::SetPosition(unsigned int a_Position)
{
	m_Position = a_Position;
}

unsigned int Texture::GetPosition() const
{
	return m_Position;
}

void Texture::UploadDDS(BaseFile* a_File, Texture::OnLoadFunction a_OnLoadFunction)
{
	static const uint32_t t_DDS = MAKEFOURCC('D', 'D', 'S', ' ');
	static const uint32_t t_DXT1 = MAKEFOURCC('D', 'X', 'T', '1');
	static const uint32_t t_DXT3 = MAKEFOURCC('D', 'X', 'T', '3');
	static const uint32_t t_DXT5 = MAKEFOURCC('D', 'X', 'T', '5');

	// Check signature. If it fails, fall back to importer
	uint32_t t_Signature;
	a_File->Get(t_Signature);
	if (t_DDS != t_Signature) return UploadRGB(a_File, a_OnLoadFunction);

	DirectX::DDS_HEADER t_Header;
	a_File->Get(t_Header);

	unsigned int t_Format;
	switch (t_Header.ddspf.fourCC)
	{

	case t_DXT1:
		t_Format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
		break;

	case t_DXT3:
		t_Format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
		break;

	case t_DXT5:
		t_Format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
		break;

	default:
		return;
	}

	GL(glGenTextures(1, &m_ID));
	GL(glBindTexture(GL_TEXTURE_2D, m_ID));
	GL(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));
	SetDefaultParameters();

	unsigned int t_BlockSize = (t_Format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) ? 8 : 16;
	unsigned int t_Height = t_Header.height;
	unsigned int t_Width = t_Header.width;
	std::vector<unsigned char> t_Buffer;

	for (unsigned int i = 0; i < std::max(1u, t_Header.mipMapCount); i++)
	{
		unsigned int t_Size = std::max(1u, ((t_Width + 3) / 4) * ((t_Height + 3) / 4)) * t_BlockSize;
		a_File->Get(t_Buffer, t_Size);

		GL(glCompressedTexImage2D(GL_TEXTURE_2D, i, t_Format, t_Width, t_Height, 0, t_Size, t_Buffer.data()));

		t_Width /= 2;
		t_Height /= 2;
	}

	if (a_OnLoadFunction) a_OnLoadFunction(this, BaseFile::LoadState::Loaded);
}

void Texture::SetDefaultParameters()
{
	GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
	GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));
	GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
	GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
}

void Texture::UploadRGB(BaseFile* a_File, Texture::OnLoadFunction a_OnLoadFunction)
{
	int t_Width, t_Height, t_BytesPerPixel;

	auto t_Data = a_File->Data();
	unsigned char *t_ImageData =
#if defined(_WIN32)
		stbi_load_from_memory((const stbi_uc*)t_Data.data(), t_Data.size(), &t_Width, &t_Height, &t_BytesPerPixel, 0);
#elif defined(__EMSCRIPTEN__)
		(unsigned char*)emscripten_get_preloaded_image_data(a_File->Name().c_str(), &t_Width, &t_Height);
	t_BytesPerPixel = 4;
#endif

	if (t_ImageData == nullptr)
	{
		if (a_OnLoadFunction) a_OnLoadFunction(nullptr, BaseFile::LoadState::FailedToLoad);
		return;
	}

#if defined(_WIN32)
	stbi_image_free(t_ImageData);
#elif defined(__EMSCRIPTEN__)
	free(t_ImageData);
#endif

	GL(glGenTextures(1, &m_ID));
	GL(glBindTexture(GL_TEXTURE_2D, m_ID));

	SetDefaultParameters();

	auto t_Format = GL_RGB;
	switch (t_BytesPerPixel)
	{
	case 3:
		t_Format = GL_RGB;
		break;

	case 4:
		t_Format = GL_RGBA;
		break;

	default:
		printf("Yikes! UploadRGB could not determine the BPP format! (%d BPP)\n", t_BytesPerPixel);
		throw 0;
		break;
	}

	GL(glTexImage2D(GL_TEXTURE_2D, 0, t_Format, t_Width, t_Height, 0, t_Format, GL_UNSIGNED_BYTE, t_ImageData));
	if (a_OnLoadFunction) a_OnLoadFunction(this, BaseFile::LoadState::Loaded);
}
