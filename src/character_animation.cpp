#include "animation_graph.hpp"
#include "character_animation.hpp"
#include "character.hpp"

#include <fnv1.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/compatibility.hpp>

namespace LeagueModel
{
	template<typename T>
	T Interpolate(T inLow, T inHigh, float inProgress)
	{
		return glm::lerp(inLow, inHigh, inProgress);
	}

	// glm::lerp between 2 quaternions are not doing what I want them to do, so I have to overwrite it for FindNearestTime.
	template<>
	glm::quat Interpolate(glm::quat inLow, glm::quat inHigh, float inProgress)
	{
		glm::quat result = glm::quat(0, 0, 0, 1);
		float dot = inLow.w * inHigh.w + inLow.x * inHigh.x + inLow.y * inHigh.y + inLow.z * inHigh.z;
		float invertedBlend = 1.0f - inProgress;

		if (dot < 0)
		{
			result.w = invertedBlend * inLow.w + inProgress * -inHigh.w;
			result.x = invertedBlend * inLow.x + inProgress * -inHigh.x;
			result.y = invertedBlend * inLow.y + inProgress * -inHigh.y;
			result.z = invertedBlend * inLow.z + inProgress * -inHigh.z;
		}
		else
		{
			result.w = invertedBlend * inLow.w + inProgress * inHigh.w;
			result.x = invertedBlend * inLow.x + inProgress * inHigh.x;
			result.y = invertedBlend * inLow.y + inProgress * inHigh.y;
			result.z = invertedBlend * inLow.z + inProgress * inHigh.z;
		}

		return glm::normalize(result);
	}

	template<typename T>
	T FindNearestTime(const std::vector<Animation::Bone::Frame<T>>& inVector, float inTime, size_t& inIndex)
	{
		auto min = inVector[0];
		auto max = inVector.back();

		if (inTime > inVector.back().time)
		{
			min = inVector[inVector.size() - 2];
		}
		else
		{
			if (inTime < inVector[inIndex].time)
				inIndex = 0;

			inIndex = inIndex ? inIndex - 1 : 0;

			for (; inIndex < inVector.size(); inIndex++)
			{
				const auto& current = inVector[inIndex];

				if (current.time <= inTime)
				{
					min = current;
					continue;
				}

				max = current;
				break;
			}
		}

		float div = max.time - min.time;
		float lerpValue = (div == 0) ? 1 : (inTime - min.time) / div;
		return Interpolate(min.frameData, max.frameData, lerpValue);
	}

	void SetupHierarchy(Character& character, glm::mat4* inBones, const Skeleton::Bone& inSkeletonBone, const glm::mat4& inParent)
	{
		static const glm::mat4 identity = glm::identity<glm::mat4>();
		glm::mat4 globalTransform;
		glm::mat4 localTransform;

		if (character.currentAnimation == nullptr) // No animation, just take identity of matrix
		{
			localTransform = identity;
			globalTransform = inParent * localTransform;
			inBones[inSkeletonBone.id] = globalTransform;
		}
		else
		{
			const Animation::Bone* animBone = character.currentAnimation ? character.currentAnimation->GetBone(inSkeletonBone.hash) : nullptr;
			if (animBone != nullptr)
			{
				glm::vec3 translation = FindNearestTime(animBone->translation, (f32)character.currentTime, character.currentFrameCache[inSkeletonBone.id].translation);
				glm::quat rotation =	FindNearestTime(animBone->rotation,	  (f32)character.currentTime, character.currentFrameCache[inSkeletonBone.id].rotation);
				glm::vec3 scale =		FindNearestTime(animBone->scale,		  (f32)character.currentTime, character.currentFrameCache[inSkeletonBone.id].scale);
				localTransform = glm::translate(translation) * glm::mat4_cast(rotation) * glm::scale(scale);
			}
			else
			{
				localTransform = identity;
			}

			globalTransform = inParent * localTransform;
			inBones[inSkeletonBone.id] = globalTransform * inSkeletonBone.inverseGlobal;
		}

		for (auto& child : inSkeletonBone.children)
			SetupHierarchy(character, inBones, *child, globalTransform);
	}

	using namespace Spek;
	using namespace LeagueLib;

	void LoadBinData(u32& inTarget, const BinVariable* inSource, u32 inDefault = 0);
	void LoadBinData(u8& inTarget, const BinVariable* inSource, u8 inDefault = 0);
	void LoadBinData(u16& inTarget, const BinVariable* inSource, u16 inDefault = 0);
	void LoadBinData(u32& inTarget, const BinVariable* inSource, u32 inDefault);
	void LoadBinData(u64& inTarget, const BinVariable* inSource, u64 inDefault = 0);
	void LoadBinData(i8& inTarget, const BinVariable* inSource, i8 inDefault = 0);
	void LoadBinData(i16& inTarget, const BinVariable* inSource, i16 inDefault = 0);
	void LoadBinData(i32& inTarget, const BinVariable* inSource, i32 inDefault = 0);
	void LoadBinData(i64& inTarget, const BinVariable* inSource, i64 inDefault = 0);
	void LoadBinData(f32& inTarget, const BinVariable* inSource, f32 inDefault = 0);
	void LoadBinData(f64& inTarget, const BinVariable* inSource, f64 inDefault = 0);
	void LoadBinData(bool& inTarget, const BinVariable* inSource, bool inDefault = 0);

	template<typename ElementType>
	void LoadBinData(std::unique_ptr<ElementType>& inTarget, const BinVariable* inSource)
	{
		if (inSource == nullptr || inSource->IsValid() == false)
		{
			inTarget = nullptr;
			return;
		}

		inTarget = std::make_unique<ElementType>();
		LoadBinData(*inTarget, inSource);
	}

	template<typename ElementType>
	void LoadBinData(std::shared_ptr<ElementType>& inTarget, const BinVariable* inSource)
	{
		if (inSource == nullptr || inSource->IsValid() == false)
		{
			inTarget = nullptr;
			return;
		}

		inTarget = std::make_shared<ElementType>();
		LoadBinData(*inTarget, inSource);
	}

	template<typename ElementType>
	void LoadBinData(std::vector<ElementType>& inTarget, const BinVariable* inSource)
	{
		inTarget.clear();
		if (inSource == nullptr || inSource->IsValid() == false)
			return;

		auto& container = *inSource->As<BinArray>();
		for (const auto& source : container)
		{
			ElementType& target = inTarget.emplace_back();
			LoadBinData(target, &source);
		}
	}

	template<typename KeyType, typename ValueType>
	void LoadBinData(std::map<KeyType, ValueType>& inTarget, const BinVariable* inSource)
	{
		inTarget.clear();
		if (inSource == nullptr)
			return;
		auto& map = inSource->As<BinMap>();
		for (const auto& source : map)
		{
			KeyType key;
			LoadBinData(key, source.first.get());

			ValueType value;
			LoadBinData(value, source.second.get());

			inTarget[key] = std::move(value);
		}
	}

	template<typename NumberType>
	void LoadNumberBinData(NumberType& inNumber, const BinVariable* inSource, NumberType inDefault)
	{
		const NumberType* result = inSource->As<NumberType>();
		inNumber = result ? *result : inDefault;
	}

	template<typename FlagType, int DefaultValue = 0, typename FieldType = u32>
	void LoadBinData(EnumBitField<FlagType, DefaultValue, FieldType>& inTarget, const BinVariable* inSource)
	{
		u32 data;
		LoadBinData(data, inSource);
		inTarget.Set(data);
	}

	void LoadBinData(std::string& inTarget, const BinVariable* inSource)
	{
		if (inSource == nullptr || inSource->IsValid() == false)
		{
			inTarget = "";
			return;
		}

		inTarget = *inSource->As<std::string>();
	}

	void LoadBinData(i8& inTarget, const BinVariable* inSource, i8 inDefault) { return LoadNumberBinData<i8>(inTarget, inSource, inDefault); }
	void LoadBinData(i16& inTarget, const BinVariable* inSource, i16 inDefault) { return LoadNumberBinData<i16>(inTarget, inSource, inDefault); }
	void LoadBinData(i32& inTarget, const BinVariable* inSource, i32 inDefault) { return LoadNumberBinData<i32>(inTarget, inSource, inDefault); }
	void LoadBinData(i64& inTarget, const BinVariable* inSource, i64 inDefault) { return LoadNumberBinData<i64>(inTarget, inSource, inDefault); }
	void LoadBinData(u8& inTarget, const BinVariable* inSource, u8 inDefault) { return LoadNumberBinData<u8>(inTarget, inSource, inDefault); }
	void LoadBinData(u16& inTarget, const BinVariable* inSource, u16 inDefault) { return LoadNumberBinData<u16>(inTarget, inSource, inDefault); }
	void LoadBinData(u32& inTarget, const BinVariable* inSource, u32 inDefault) { return LoadNumberBinData<u32>(inTarget, inSource, inDefault); }
	void LoadBinData(u64& inTarget, const BinVariable* inSource, u64 inDefault) { return LoadNumberBinData<u64>(inTarget, inSource, inDefault); }
	void LoadBinData(f64& inTarget, const BinVariable* inSource, f64 inDefault)  { return LoadNumberBinData<f64> (inTarget, inSource, inDefault); }

	void LoadBinData(f32& inTarget, const BinVariable* inSource, f32 inDefault)
	{
		f64 dummy;
		LoadNumberBinData(dummy, inSource, (f64)inDefault);
		inTarget = (f32)dummy;
	}

	void LoadBinData(bool& inTarget, const BinVariable* inSource, bool inDefault)
	{
		u8 dummy;
		LoadNumberBinData(dummy, inSource, (u8)inDefault);
		inTarget = (bool)dummy;
	}

#define LOAD_BIN_DATA_NAME(FIELDNAME)              do { LoadBinData(inTarget.FIELDNAME, inSource ? &(*inSource)[#FIELDNAME] : nullptr); } while (0);
#define LOAD_BIN_DATA_ENUM(FIELDNAME)              do { LoadBinData((u32&)inTarget.FIELDNAME, inSource ? &(*inSource)[#FIELDNAME] : nullptr); } while (0);
#define LOAD_BIN_DATA_NAME_DEF(FIELDNAME, DEFAULT) do { LoadBinData(inTarget.FIELDNAME, inSource ? &(*inSource)[#FIELDNAME] : nullptr, DEFAULT); } while (0);

	void LoadEventMap(std::map<StringHash, std::unique_ptr<AnimationBaseEventData>>& inTarget, const BinMap* inEventMap)
	{
		inTarget.clear();
		if (inEventMap == nullptr)
			return;

		for (auto& [keyVar, event ] : *inEventMap)
		{
			StringHash key = *keyVar.As<StringHash>();
			const BinObject* ev = event.As<BinObject>();

			StringHash typeHash = ev->GetTypeHash();
			switch (typeHash)
			{
			case FNV1Hash("ParticleEventData"): // TODO
			case FNV1Hash("SoundEventData"): // TODO
			case FNV1Hash("FaceCameraEventData"): // TODO
			case FNV1Hash("ConformToPathEventData"): // TODO
			case FNV1Hash("JointSnapEventData"): // TODO
				break;

			case FNV1Hash("SubmeshVisibilityEventData"):
			{
				std::unique_ptr<AnimationSubmeshVisibilityEventData> pointer = std::make_unique<AnimationSubmeshVisibilityEventData>();
				LoadBinData(*pointer, &event);
				inTarget[key] = std::move(pointer);
				break;
			}
			}
		}
	}

	void LoadBinData(AnimationTrackBlendMode& inTarget, const BinVariable* inSource)
	{
		LoadBinData((u32&)inTarget, inSource, (u32)AnimationTrackBlendMode::kAdditiveTrack);
	}

	void LoadBinData(AnimationSyncGroupDataType& inTarget, const BinVariable* inSource)
	{
		LoadBinData((u32&)inTarget, inSource, (u32)AnimationSyncGroupDataType::kTime);
	}

	void LoadBinData(AnimationParametricUpdaterType& inTarget, const BinVariable* inSource)
	{
		LoadBinData((u32&)inTarget, inSource, (u32)AnimationParametricUpdaterType::kIsTurning);
	}

	void LoadBinData(AnimationBaseEventData& inTarget, const BinVariable* inSource)
	{
		LOAD_BIN_DATA_NAME(mIsSelfOnly);
		LOAD_BIN_DATA_NAME(mFireIfAnimationEndsEarly);
		LOAD_BIN_DATA_NAME_DEF(mStartFrame, -1.0f);
		LOAD_BIN_DATA_NAME_DEF(mEndFrame, -1.0f);
		LOAD_BIN_DATA_NAME(mName);
	}

	void LoadBinData(AnimationSubmeshVisibilityEventData& inTarget, const BinVariable* inSource)
	{
		LoadBinData((AnimationBaseEventData&)inTarget, inSource);
		LOAD_BIN_DATA_NAME(mShowSubmeshList);
		LOAD_BIN_DATA_NAME(mHideSubmeshList);
	}

	void LoadBinData(AnimationSyncGroupData& inTarget, const BinVariable* inSource)
	{
		LOAD_BIN_DATA_ENUM(mType);
		LOAD_BIN_DATA_NAME(mName);
	}

	void LoadBinData(AnimationTrackData& inTarget, const BinVariable* inSource)
	{
		LOAD_BIN_DATA_NAME(mPriority);
		LOAD_BIN_DATA_NAME(mBlendWeight);
		LOAD_BIN_DATA_ENUM(mBlendMode);
		LOAD_BIN_DATA_NAME(mIndex);
		LOAD_BIN_DATA_NAME(mName);
	}

	void LoadBinData(AnimationParametricPairData& inTarget, const BinVariable* inSource)
	{
		LOAD_BIN_DATA_NAME(mValue);
		LOAD_BIN_DATA_NAME(mClipName);
	}

	void LoadBinData(AnimationMaskData& inTarget, const BinVariable* inSource)
	{
		LOAD_BIN_DATA_NAME(mID);
		LOAD_BIN_DATA_NAME(mName);
	}

	void LoadBinData(AnimationSelectorPairData& inTarget, const BinVariable* inSource)
	{
		LOAD_BIN_DATA_NAME(mClipName);
		LOAD_BIN_DATA_NAME(mProbability);
		LOAD_BIN_DATA_NAME(mClipID);
	}

	void LoadBinData(AnimationConditionFloatPairData& inTarget, const BinVariable* inSource)
	{
		LOAD_BIN_DATA_NAME(mClipName);
		LOAD_BIN_DATA_NAME(mValue);
		LOAD_BIN_DATA_NAME(mHoldAnimationToHigher);
		LOAD_BIN_DATA_NAME(mHoldAnimationToLower);
		LOAD_BIN_DATA_NAME(mClipID);
	}

	void LoadBinData(AnimationClipResourceData& inTarget, const BinVariable* inSource)
	{
		LOAD_BIN_DATA_NAME(mAnimationFilePath);
	}

	void LoadBinData(AnimationClipBaseData& inTarget, const BinVariable* inSource)
	{
		LOAD_BIN_DATA_NAME(mAnimationInterruptionGroupNames);
		LOAD_BIN_DATA_NAME(mAnimationResourceData);
		LOAD_BIN_DATA_NAME(mFlags);
		LOAD_BIN_DATA_NAME(mID);
		LOAD_BIN_DATA_NAME(mName);
		LoadEventMap(inTarget.mEventDataMap, inSource ? (*inSource)["mEventDataMap"].As<BinMap>() : nullptr);

		inTarget.Type = AnimationClipType::Atomic;
	}

	void LoadBinData(AnimationSelectorClipData& inTarget, const BinVariable* inSource)
	{
		LoadBinData((AnimationAtomicClipData&)inTarget, inSource);
		LOAD_BIN_DATA_NAME(mSelectorPairDataList);

		inTarget.Type = AnimationClipType::Selector;
	}

	void LoadBinData(AnimationParallelClipData& inTarget, const BinVariable* inSource)
	{
		LoadBinData((AnimationAtomicClipData&)inTarget, inSource);
		LOAD_BIN_DATA_NAME(mClipNameList);
		LOAD_BIN_DATA_NAME(mClipIndexList);

		inTarget.Type = AnimationClipType::Parrallel;
	}

	void LoadBinData(AnimationSequencerClipData& inTarget, const BinVariable* inSource)
	{
		LoadBinData((AnimationAtomicClipData&)inTarget, inSource);
		LOAD_BIN_DATA_NAME(mClipNameList);
		LOAD_BIN_DATA_NAME(mClipIndexList);

		inTarget.Type = AnimationClipType::Sequencer;
	}

	void LoadBinData(AnimationBlendableClipData& inTarget, const BinVariable* inSource)
	{
		LoadBinData((AnimationAtomicClipData&)inTarget, inSource);
		LOAD_BIN_DATA_NAME(mMaskDataName);
		LOAD_BIN_DATA_NAME(mTrackDataName);
		LOAD_BIN_DATA_NAME(mSyncGroupDataName);

		__debugbreak();
		// LOAD_BIN_DATA_NAME(mMaskData);
		// LOAD_BIN_DATA_NAME(mTrackData);
		// LOAD_BIN_DATA_NAME(mSyncGroupData);

		inTarget.Type = AnimationClipType::Blendable;
	}

	void LoadBinData(AnimationConditionBoolClipData& inTarget, const BinVariable* inSource)
	{
		LoadBinData((AnimationAtomicClipData&)inTarget, inSource);
		LOAD_BIN_DATA_NAME(mUpdaterType);
		LOAD_BIN_DATA_NAME(mChangeAnimationMidPlay);
		LOAD_BIN_DATA_NAME(mPlayAnimChangeFromBeginning);
		LOAD_BIN_DATA_NAME(mChildAnimDelaySwitchTime);
		LOAD_BIN_DATA_NAME(mTrueConditionClipName);
		LOAD_BIN_DATA_NAME(mFalseConditionClipName);
		LOAD_BIN_DATA_NAME(mTrueConditionClipID);
		LOAD_BIN_DATA_NAME(mFalseConditionClipID);

		inTarget.Type = AnimationClipType::Atomic;
	}

	void LoadBinData(AnimationConditionFloatClipData& inTarget, const BinVariable* inSource)
	{
		LoadBinData((AnimationAtomicClipData&)inTarget, inSource);
		LOAD_BIN_DATA_NAME(mUpdaterType);
		LOAD_BIN_DATA_NAME(mChangeAnimationMidPlay);
		LOAD_BIN_DATA_NAME(mPlayAnimChangeFromBeginning);
		LOAD_BIN_DATA_NAME(mChildAnimDelaySwitchTime);
		LOAD_BIN_DATA_NAME(mConditionFloatPairDataList);

		inTarget.Type = AnimationClipType::ConditionFloat;
	}

	void LoadBinData(AnimationParametricClipData& inTarget, const BinVariable* inSource)
	{
		LoadBinData((AnimationAtomicClipData&)inTarget, inSource);
		LOAD_BIN_DATA_NAME(mUpdaterType);
		LOAD_BIN_DATA_NAME(mTrackDataName);
		LOAD_BIN_DATA_NAME(mConditionFloatPairDataList);

		inTarget.Type = AnimationClipType::Parametric;
	}

	AnimationAtomicClipData* CreateClip(uint32_t inClipHash, const BinVariable& inClip, AnimationGraph& inGraph, const BinMap* inClipData, const BinMap* inMaskDataMap, const BinMap* inTrackDataMap)
	{
#define INIT_CLIP(TYPE) \
		{\
			auto clip = std::make_shared<TYPE>();\
			LoadBinData(*clip, &inClip);\
			inGraph.clips[inClipHash] = clip;\
			return clip.get();\
		}

		if (inGraph.clips[inClipHash] != nullptr)
			return inGraph.clips[inClipHash].get();

		uint32_t type = inClip.As<BinObject>()->GetTypeHash();
		switch (type)
		{
		case FNV1Hash("AtomicClipData"):
			INIT_CLIP(AnimationAtomicClipData);

		case FNV1Hash("SelectorClipData"):
			INIT_CLIP(AnimationSelectorClipData);

		case FNV1Hash("ParallelClipData"):
			INIT_CLIP(AnimationParallelClipData);

		case FNV1Hash("SequencerClipData"):
			INIT_CLIP(AnimationSequencerClipData);

		case FNV1Hash("ConditionFloatClipData"):
			INIT_CLIP(AnimationConditionFloatClipData);

		case FNV1Hash("ConditionBoolClipData"):
			INIT_CLIP(AnimationConditionBoolClipData);

		case FNV1Hash("ParametricClipData"):
			INIT_CLIP(AnimationParametricClipData);

		default:
			__debugbreak();
			return nullptr;
		}

#undef INIT_CLIP
	}

	void ApplyEvent(Character& character, AnimationSubmeshVisibilityEventData& inEvent, bool inShouldSwap)
	{
		for (auto submeshToShow : inEvent.mShowSubmeshList)
		{
			auto index = character.meshMap.find(submeshToShow);
			if (index != character.meshMap.end())
				index->second->shouldRender = inShouldSwap;
		}

		for (auto submeshToHide : inEvent.mHideSubmeshList)
		{
			auto index = character.meshMap.find(submeshToHide);
			if (index != character.meshMap.end())
				index->second->shouldRender = !inShouldSwap;
		}
	}

#undef LOAD_BIN_DATA_NAME
}