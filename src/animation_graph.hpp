#pragma once

#include "league_lib/util/enum_bitfield.hpp"
#include "animation_flags.hpp"
#include "character_animation.hpp"

#include <spek/file/file.hpp>

#include <string>
#include <map>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace LeagueModel
{
	class Skeleton;

	struct BlendData
	{
		f32 time;
		AnimationClipBaseData* clip;
	};

	class AnimationGraph
	{
	public:
		class Track {};

		using Clip = AnimationClipBaseData;
		using ClipRefPair = std::pair<const Clip*, const Clip*>;

		std::map<u32, std::shared_ptr<Clip>> clips;
		std::map<ClipRefPair, BlendData> blendData;

		Clip* GetClip(u32 inHash) const;
		void AddBlendData(const Clip& inFromClip, const Clip& inToClip, f32 inTime, Clip* inBlendClip);
	};
}