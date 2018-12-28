#pragma once

#include <helper.hpp>
#include <league_model/file/file_system.hpp>

#include <glm/glm.hpp>

#include <map>

class Texture
{
public:
	using OnLoadFunction = void (*)(Texture& a_Texture, void* a_Argument);
	~Texture();

	void Load(const std::string& a_ImagePath, Texture::OnLoadFunction a_OnLoadFunction = nullptr, void* a_Argument = nullptr);

	glm::vec2 GetDimensions() const;

	operator int() const;
		
	void SetPosition(unsigned int a_Position);
	unsigned int GetPosition() const;

	File::LoadState GetLoadState() const;

private:
	void SetDefaultParameters();
	File::LoadState UploadRGB(File* a_File);
	File::LoadState UploadDDS(File* a_File);

	File::LoadState m_LoadState = File::LoadState::NotLoaded;
	unsigned int m_ID;
	unsigned int m_Position = 0;
	glm::vec2 m_Dimensions;
};