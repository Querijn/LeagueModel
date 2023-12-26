#include "animation_graph.hpp"
#include "animation.hpp"

namespace LeagueModel
{
	using namespace Spek;

	AnimationClipBaseData* AnimationGraph::GetClip(u32 inHash) const
	{
		auto index = clips.find(inHash);
		if (index == clips.end())
			return nullptr;

		return index->second.get();
	}

	void AnimationGraph::AddBlendData(const Clip& inFromClip, const Clip& inToClip, f32 inTime, Clip* inBlendClip)
	{
		blendData[{ &inFromClip, & inToClip }] = { inTime, inBlendClip };
	}
}
