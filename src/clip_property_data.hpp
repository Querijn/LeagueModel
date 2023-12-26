#pragma once

#include "model/flags.hpp"
#include "util/enum_bitfield.hpp"

#include <spek/file/file.hpp>
#include <spek/animation/animation.hpp>

#include <string>
#include <map>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace LeagueModel
{
	class Skeleton;

	struct AnimationClipBaseData;
	using AtomicClipData = AnimationClipBaseData;
	struct AnimationMaskData;
	struct AnimationTrackData;
	struct AnimationSyncGroupData;

	struct AnimationClipBaseData
	{
		StringHash mName;
		EnumBitField<enum AnimationClipDataFlags, 0> mFlags;
		std::vector<StringHash> mAnimationInterruptionGroupNames;
		u32 mID;
	};

	struct AnimationBaseEventData
	{
		bool mIsSelfOnly;
		bool mFireIfAnimationEndsEarly;
		float mStartFrame;
		float mEndFrame;
		StringHash mName;
	};

	struct AnimationBlendableClipData : AnimationClipBaseData
	{
		StringHash mMaskDataName;
		StringHash mTrackDataName;
		StringHash mSyncGroupDataName;
		std::map<StringHash, std::unique_ptr<AnimationBaseEventData>> mEventDataMap;
		AnimationMaskData* mMaskData;
		AnimationTrackData* mTrackData;
		AnimationSyncGroupData* mSyncGroupData;
	};

	struct AnimationConditionFloatPairData
	{
		StringHash mClipName;
		float mValue;
		float mHoldAnimationToHigher;
		float mHoldAnimationToLower;
		unsigned int mClipID;
	};

	struct AnimationConditionFloatClipData : AnimationClipBaseData
	{
		AnimationParametricUpdaterType mUpdaterType;
		bool mChangeAnimationMidPlay;
		bool mPlayAnimChangeFromBeginning;
		float mChildAnimDelaySwitchTime;
		std::vector<AnimationConditionFloatPairData> mConditionFloatPairDataList;
	};

	struct AnimationBaseBlendData {};

	struct AnimationTransitionClipBlendData : AnimationBaseBlendData
	{
		StringHash mClipName;
		AnimationClipBaseData* mClipData;
	};

	struct AnimationConditionBoolClipData : AnimationClipBaseData
	{
		AnimationParametricUpdaterType mUpdaterType;
		bool mChangeAnimationMidPlay;
		bool mPlayAnimChangeFromBeginning;
		float mChildAnimDelaySwitchTime;
		StringHash mTrueConditionClipName;
		StringHash mFalseConditionClipName;
		u32 mTrueConditionClipID;
		u32 mFalseConditionClipID;
	};

	struct AnimationSequencerClipData : AnimationClipBaseData
	{
		std::vector<StringHash> mClipNameList;
		std::vector<u32> mClipIndexList;
	};

	struct AnimationSelectorPairData
	{
		StringHash mClipName;
		float mProbability;
		u32 mClipID;
	};

	struct AnimationSelectorClipData : AnimationClipBaseData
	{
		std::vector<AnimationSelectorPairData> mSelectorPairDataList;
	};

	struct AnimationParallelClipData : AnimationClipBaseData
	{
		std::vector<StringHash> mClipNameList;
		std::vector<u32> mClipIndexList;
	};
}