#pragma once

#include "types.hpp"

#include <vector>
#include <map>
#include <spek/file/file.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace LeagueModel
{
	struct Animation
	{
		struct Bone
		{
			template<typename T>
			struct Frame
			{
				Frame(float inTime = 0, T inFrameData = T()) :
					time(inTime), frameData(inFrameData)
				{
				}

				float time;
				T frameData;
			};
			using TranslationFrame = Frame<glm::vec3>;
			using RotationFrame = Frame<glm::quat>;
			using ScaleFrame = Frame<glm::vec3>;

			u32 hash = 0;

			std::vector<TranslationFrame> translation;
			std::vector<RotationFrame> rotation;
			std::vector<ScaleFrame> scale;
		};

		using OnLoadFunction = std::function<void(Animation& animation)>;
		void Load(const std::string& inFilePath, OnLoadFunction inOnLoadFunction = nullptr);
		const Bone* GetBone(u32 hash) const;

		Spek::File::LoadState loadState = Spek::File::LoadState::NotLoaded;
		float fps, duration;
		std::vector<Bone> bones;
		std::string name;

	private:
		Spek::File::Handle file;
		Spek::File::LoadState LoadVersion1(Spek::File::Handle inFile, size_t& inOffset);
		Spek::File::LoadState LoadVersion3(Spek::File::Handle inFile, size_t& inOffset);
		Spek::File::LoadState LoadVersion4(Spek::File::Handle inFile, size_t& inOffset);
		Spek::File::LoadState LoadVersion5(Spek::File::Handle inFile, size_t& inOffset);
	};
}