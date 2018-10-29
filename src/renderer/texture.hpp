#pragma once

#include <helper.hpp>
#include <file_system.hpp>

#include <glm/glm.hpp>

#include <map>
#include <functional>

class Texture
{
public:
	using OnLoadFunction = std::function<void(Texture* a_Texture, BaseFile::LoadState a_LoadState)>;
	Texture();
	~Texture();

	void Load(const std::string& a_ImagePath, Texture::OnLoadFunction a_OnLoadFunction = nullptr);

	glm::vec2 GetDimensions() const;

	operator int() const;
		
	void SetPosition(unsigned int a_Position);
	unsigned int GetPosition() const;

private:
	void SetDefaultParameters();
	void UploadRGB(BaseFile* a_File, Texture::OnLoadFunction a_OnLoadFunction);
	void UploadDDS(BaseFile* a_File, Texture::OnLoadFunction a_OnLoadFunction);

	unsigned int m_ID;
	unsigned int m_Position = 0;
	glm::vec2 m_Dimensions;
};