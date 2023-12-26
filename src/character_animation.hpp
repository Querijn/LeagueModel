#pragma once

#include "animation_flags.hpp"
#include "skeleton.hpp"

#include "league_lib/bin/bin_valuestorage.hpp"
#include "league_lib/util/enum_bitfield.hpp"

#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>
#include <memory>
#include <unordered_map>

namespace LeagueModel
{
	enum AnimationClipDataFlags {}; // TODO

	enum class AnimationClipType
	{
		Atomic,
		Selector,
		Parrallel,
		Sequencer,
		Blendable,
		ConditionBool,
		ConditionFloat,
		Parametric
	};

	enum class AnimationEventType
	{
		SubmeshVisibilityEvent
	};

	struct AnimationClipResourceData
	{
		std::string mAnimationFilePath;
	};

	struct AnimationBaseEventData
	{
		AnimationEventType Type;
		AnimationBaseEventData(AnimationEventType inType) : Type(inType) {}

		bool mIsSelfOnly;
		bool mFireIfAnimationEndsEarly;
		float mStartFrame;
		float mEndFrame;
		StringHash mName;

	private:
		AnimationBaseEventData() = delete;
	};

	struct AnimationSubmeshVisibilityEventData : public AnimationBaseEventData
	{
		AnimationSubmeshVisibilityEventData() : AnimationBaseEventData(AnimationEventType::SubmeshVisibilityEvent) {}

		std::vector<StringHash> mShowSubmeshList;
		std::vector<StringHash> mHideSubmeshList;
	};

	struct AnimationClipBaseData
	{
		AnimationClipResourceData mAnimationResourceData;
		AnimationClipType Type;

		std::map<StringHash, std::unique_ptr<AnimationBaseEventData>> mEventDataMap;
		StringHash mName;
		LeagueLib::EnumBitField<AnimationClipDataFlags, 0> mFlags;
		std::vector<StringHash> mAnimationInterruptionGroupNames;
		u32 mID;
	};

	struct TransformData
	{
		glm::vec3 mPosition;
		glm::vec3 mScale;
		glm::quat mOrientation;
	};

	struct AnimationJoint
	{
		u16 mIndex;
		std::string mName;
		unsigned int mNameHash;
		u16 mFlags;
		float mRadius;
		i16 mParentIndex;
		TransformData mParentOffset;
		TransformData mInverseRootOffset;
	};

	struct AnimationRigResource
	{
		std::string mName;
		std::string mAssetName;
		u16 mFlags;
		std::vector<AnimationJoint> mJointList;
		std::vector<unsigned short> mShaderJointList;
		std::unordered_map<StringHash, short> mNameToIndexMap;
		std::unordered_map<unsigned int, short> mHashToIndexMap;
	};

	struct AnimationSyncGroupData
	{
		AnimationSyncGroupDataType mType;
		StringHash mName;
	};

	struct AnimationMaskData
	{
		unsigned int mID;
		std::vector<float> mWeightList;
		StringHash mName;
	};

	struct AnimationTrackData
	{
		unsigned int mPriority;
		float mBlendWeight;
		AnimationTrackBlendMode mBlendMode;
		unsigned int mIndex;
		StringHash mName;
	};

	struct AnimationClipController
	{
		float mCurrentTime;
		float mSpeedRatio;
		u32 mPlaybackFlags;
		bool mIgnoreLooping;
		std::unique_ptr<AnimationRigResource> mRigResource;
		std::unique_ptr<AnimationClipBaseData> mClipBaseData;
	};

	struct AnimationBlendableClipData : AnimationClipBaseData
	{
		StringHash mMaskDataName;
		StringHash mTrackDataName;
		StringHash mSyncGroupDataName;
		std::unique_ptr<AnimationMaskData> mMaskData;
		std::unique_ptr<AnimationTrackData> mTrackData;
		std::unique_ptr<AnimationSyncGroupData> mSyncGroupData;
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
		std::unique_ptr<AnimationClipBaseData> mClipData;
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

	struct AnimationParametricPairData
	{
		std::string mClipName;
		f32 mValue;
	};

	struct AnimationParametricClipData : AnimationClipBaseData
	{
		AnimationParametricUpdaterType mUpdaterType;
		StringHash mTrackDataName;
		std::vector<AnimationParametricPairData> mConditionFloatPairDataList;
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

	struct AnimationGraphData
	{
		bool mUseCascadeBlend;
		float mCascadeBlendValue;
		std::map<StringHash, std::unique_ptr<AnimationClipBaseData>> mClipDataMap;
		std::map<StringHash, AnimationMaskData> mMaskDataMap;
		std::map<StringHash, AnimationTrackData> mTrackDataMap;
		std::map<StringHash, AnimationSyncGroupData> mSyncGroupDataMap;
		std::map<u64, std::unique_ptr<AnimationBaseBlendData>> mBlendDataTable;
		std::unordered_map<u32, AnimationClipBaseData const*> mIDToClipMap;
		std::unordered_map<StringHash, u32> mNameToIDMap;
		bool mHasEventData;
	};

	class AnimationBlendData
	{

	};

	using AnimationAtomicClipData = AnimationClipBaseData;

	void LoadBinData(AnimationTrackBlendMode& inTarget, const LeagueLib::BinVariable* inSource);
	void LoadBinData(AnimationSyncGroupDataType& inTarget, const LeagueLib::BinVariable* inSource);
	void LoadBinData(AnimationParametricUpdaterType& inTarget, const LeagueLib::BinVariable* inSource);
	void LoadBinData(AnimationBaseEventData& inTarget, const LeagueLib::BinVariable* inSource);
	void LoadBinData(AnimationSubmeshVisibilityEventData& inTarget, const LeagueLib::BinVariable* inSource);
	void LoadBinData(AnimationSyncGroupData& inTarget, const LeagueLib::BinVariable* inSource);
	void LoadBinData(AnimationTrackData& inTarget, const LeagueLib::BinVariable* inSource);
	void LoadBinData(AnimationMaskData& inTarget, const LeagueLib::BinVariable* inSource);
	void LoadBinData(AnimationSelectorPairData& inTarget, const LeagueLib::BinVariable* inSource);
	void LoadBinData(AnimationConditionFloatPairData& inTarget, const LeagueLib::BinVariable* inSource);
	void LoadBinData(AnimationClipResourceData& inTarget, const LeagueLib::BinVariable* inSource);
	void LoadBinData(AnimationClipBaseData& inTarget, const LeagueLib::BinVariable* inSource);
	void LoadBinData(AnimationSelectorClipData& inTarget, const LeagueLib::BinVariable* inSource);
	void LoadBinData(AnimationParallelClipData& inTarget, const LeagueLib::BinVariable* inSource);
	void LoadBinData(AnimationSequencerClipData& inTarget, const LeagueLib::BinVariable* inSource);
	void LoadBinData(AnimationBlendableClipData& inTarget, const LeagueLib::BinVariable* inSource);
	void LoadBinData(AnimationConditionBoolClipData& inTarget, const LeagueLib::BinVariable* inSource);
	void LoadBinData(AnimationConditionFloatClipData& inTarget, const LeagueLib::BinVariable* inSource);

	class AnimationGraph;
	class Character;

	template<typename ClipType>
	void InitClip(AnimationGraph& inGraph, uint32_t inClipHash, const LeagueLib::BinVariable& inSourceClip);
	AnimationAtomicClipData* CreateClip(uint32_t inClipHash, const LeagueLib::BinVariable& inClip, AnimationGraph& inGraph, const LeagueLib::BinMap* inClipData, const LeagueLib::BinMap* inMaskDataMap, const LeagueLib::BinMap* inTrackDataMap);

	void ApplyEvent(Character& character, AnimationSubmeshVisibilityEventData& inEvent, bool inShouldSwap);
	void SetupHierarchy(Character& character, glm::mat4* inBones, const Skeleton::Bone& inSkeletonBone, const glm::mat4& inParent);
}