#pragma once

#include <spek/file/file.hpp>
#include <sokol_gfx.h>
#include <functional>

namespace LeagueModel
{
	struct ManagedImage
	{
		using OnLoadFunction = std::function<void(ManagedImage& skin)>;
		ManagedImage(const char* path, OnLoadFunction onImageLoaded = nullptr);
		~ManagedImage();

		sg_image image = {};
		Spek::File::LoadState loadState = Spek::File::LoadState::NotLoaded;

	private:
		Spek::File::Handle file = nullptr;
	};
}
	