#pragma once

#include <file/base_file.hpp>

#include <string>

class Image
{
public:
	enum Type
	{
		FOUR_CC,
		DEFAULT
	};

	using OnLoadFunction = std::function<void(Image* a_Image, BaseFile::LoadState a_State)>;

	~Image() {}

	const uint8_t* Data() const;
	Type GetType() const;

	friend class NativeFileSystem;
protected:
	Image(const std::string& a_File, const Image::OnLoadFunction& a_OnLoadFunction = nullptr);

private:
	std::vector<uint8_t> m_Data;
	Type m_Type;
};
