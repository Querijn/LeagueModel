#include <renderer/opengl.hpp>
#include <renderer/texture.hpp>

#include <profiling/memory.hpp>
#include <dds.h>

#include <algorithm>

void Texture::Load(const std::string& a_ImagePath, Texture::OnLoadFunction a_OnLoadFunction, void* a_Argument)
{
	auto* t_File = FileSystem::GetFile(a_ImagePath);

	struct LoadData
	{
		LoadData(Texture* a_Target, OnLoadFunction a_Function, void* a_Argument) :
			Target(a_Target), OnLoadFunction(a_Function), Argument(a_Argument)
		{}

		Texture* Target;
		OnLoadFunction OnLoadFunction;
		void* Argument;
	};
	auto* t_LoadData = LM_NEW(LoadData(this, a_OnLoadFunction, a_Argument));
		
	t_File->Load([](File* a_File, File::LoadState a_LoadState, void* a_Argument)
	{
		auto* t_LoadData = (LoadData*)a_Argument;
		auto* t_Texture = t_LoadData->Target;
		if (a_LoadState != File::LoadState::Loaded)
		{
			t_Texture->m_LoadState = a_LoadState;
			if (t_LoadData->OnLoadFunction) t_LoadData->OnLoadFunction(*t_Texture, t_LoadData->Argument);

			// FileSystem::CloseFile(*a_File);
			LM_DEL(t_LoadData);
			return;
		}

		std::string t_Path = a_File->GetName();
		printf("Loading texture %s\n", t_Path.c_str());

		// Check if image path ends in DDS
		auto t_Index = t_Path.find_last_of('.');
		auto t_IsDDS = (t_Path[t_Index + 1] == 'd' || t_Path[t_Index + 1] == 'D') && (t_Path[t_Index + 2] == 'd' || t_Path[t_Index + 2] == 'D') && (t_Path[t_Index + 3] == 's' || t_Path[t_Index + 3] == 'S');

		// Upload as DDS or as whatever STB can decypher for us
		t_Texture->m_LoadState = t_IsDDS ? t_Texture->UploadDDS(a_File) : t_Texture->UploadRGB(a_File);

		if (t_LoadData->OnLoadFunction) t_LoadData->OnLoadFunction(*t_Texture, t_LoadData->Argument);
		LM_DEL(t_LoadData);
	}, t_LoadData);
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

File::LoadState Texture::GetLoadState() const
{
	return m_LoadState;
}

File::LoadState Texture::UploadDDS(File* a_File)
{
	static const uint32_t t_DDS = MAKEFOURCC('D', 'D', 'S', ' ');
	static const uint32_t t_DXT1 = MAKEFOURCC('D', 'X', 'T', '1');
	static const uint32_t t_DXT3 = MAKEFOURCC('D', 'X', 'T', '3');
	static const uint32_t t_DXT5 = MAKEFOURCC('D', 'X', 'T', '5');

	// Check signature. If it fails, fall back to importer
	uint32_t t_Signature;
	size_t t_Offset = 0;
	a_File->Get(t_Signature, t_Offset);
	if (t_DDS != t_Signature) return UploadRGB(a_File);

	DirectX::DDS_HEADER t_Header;
	a_File->Get(t_Header, t_Offset);

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
		return File::LoadState::FailedToLoad;
	}

	GL(glGenTextures(1, &m_ID));
	GL(glBindTexture(GL_TEXTURE_2D, m_ID));
	GL(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));
	SetDefaultParameters();

	unsigned int t_BlockSize = (t_Format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) ? 8 : 16;
	unsigned int t_Height = t_Header.height;
	unsigned int t_Width = t_Header.width;
	std::vector<uint8_t> t_Buffer;

	for (unsigned int i = 0; i < std::max(1u, t_Header.mipMapCount); i++)
	{
		unsigned int t_Size = std::max(1u, ((t_Width + 3) / 4) * ((t_Height + 3) / 4)) * t_BlockSize;
		a_File->Get(t_Buffer, t_Size, t_Offset);

		GL(glCompressedTexImage2D(GL_TEXTURE_2D, i, t_Format, t_Width, t_Height, 0, t_Size, t_Buffer.data()));

		t_Width /= 2;
		t_Height /= 2;
	}

	return File::LoadState::Loaded;
}

void Texture::SetDefaultParameters()
{
	GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
	GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));
	GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
	GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
}

File::LoadState Texture::UploadRGB(File* a_File)
{
	int t_Width, t_Height, t_BytesPerPixel;

	auto t_Data = a_File->GetData();
	unsigned char *t_ImageData = stbi_load_from_memory((const stbi_uc*)t_Data.data(), t_Data.size(), &t_Width, &t_Height, &t_BytesPerPixel, 0);

	if (t_ImageData == nullptr) return File::LoadState::FailedToLoad;

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
		return File::LoadState::FailedToLoad;
	}

	GL(glTexImage2D(GL_TEXTURE_2D, 0, t_Format, t_Width, t_Height, 0, t_Format, GL_UNSIGNED_BYTE, t_ImageData));

	stbi_image_free(t_ImageData);

	return File::LoadState::Loaded;
}
