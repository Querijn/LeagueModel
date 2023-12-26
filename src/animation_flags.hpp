#pragma once

#include <spek/util/types.hpp>

namespace LeagueModel
{
	using StringHash = u32;

	enum AnimationBlendFlag
	{
		Align = 0x1,
	};

	enum AnimationEventFlags
	{
		Loop		= 0b0001,
		KillEvent	= 0b0010,
		Detachable	= 0b0100,
		SelfOnly	= 0b1000,
		Default		= Loop | KillEvent,
	};

	enum AnimationClipTypes
	{
		eClipTypeInvalid = 0,
		eAtomic = 1,
		eSelector = 2,
		eSequencer = 3,
		eParallel = 4,
		eMultiChildClip = 5,
		eParametric = 6,
		eConditionBool = 7,
		eConditionFloat = 8,
	};

	enum AnimationChannelTypes
	{
		eUnknown = 0x0,
		eFloat1 = 0x1,
		eFloat2 = 0x2,
		eFloat3 = 0x3,
		eQuaternion = 0x4,
		eFloatZero1 = 0x5,
		eFloatZero2 = 0x6,
		eFloatZero3 = 0x7,
		eFloatOne1 = 0x8,
		eFloatOne2 = 0x9,
		eFloatOne3 = 0xA,
		eQuaternionIdentity = 0xB,
	};

	enum RigPoseModifierType
	{
	  kLockRootOrientationModifier = 0x0,
	  kSyncedAnimationModifier = 0x1,
	  kConformToPathRigPoseModifier = 0x2,
	  kJointSnapModifier = 0x3,
	  kVertexAnimationModifier = 0x4,
	};

	enum AnimationPlaybackFlags
	{
		PLAYBACK_DEFAULT				= 0b0000000000,
		PLAYBACK_LOOPED					= 0b0000000001,
		PLAYBACK_PAUSEONEND				= 0b0000000010,
		PLAYBACK_NEXT					= 0b0000000100,
		PLAYBACK_INTERRUPTIBLE			= 0b0000001000,
		PLAYBACK_LOCK					= 0b0000010000,
		PLAYBACK_NOBLEND				= 0b0000100000,
		PLAYBACK_IDLELOCKED				= 0b0001000000,
		PLAYBACK_TRANSITION				= 0b0010000000,
		PLAYBACK_DEFAULTPARAMETRICVALUE	= 0b0100000000,
		PLAYBACK_IGNORE_CLIP_PLAYED		= 0b1000000000,
	};

	enum AnimationChannelSubTarget
	{
		eSubTargetUnknown = 0,
		eTranslation = 1,
		eRotation = 2,
		eScale = 3,
		eTranslationX = 4,
		eTranslationY = 5,
		eTranslationZ = 6,
		eRotationX = 7,
		eRotationY = 8,
		eRotationZ = 9,
		eScaleX = 10,
		eScaleY = 11,
		eScaleZ = 12,
	};

	enum AnimationInterpolationMode
	{
		ANIMATION_INTERPOLATION_NONE = 0x0,
		ANIMATION_INTERPOLATION_NLERP = 0x1,
		ANIMATION_INTERPOLATION_SLERP = 0x2,
		ANIMATION_INTERPOLATION_COUNT = 0x3,
	};

	enum AnimationParametricUpdaterType
	{
		kInvalidUpdaterType = 0xFFFFFFFF,
		kLookAtSpellTargetAngle = 0x0,
		kSkinScale = 0x1,
		kDisplacement = 0x2,
		kTurnAngle = 0x3,
		kMoveSpeed = 0x4,
		kIsMoving = 0x5,
		kFacing = 0x6,
		kLookAtInterestAngle = 0x7,
		kParBarPercent = 0x8,
		kLookAtInterestDistance = 0x9,
		kAttackSpeed = 0xA,
		kSlopeAngle = 0xB,
		kLookAtSpellTargetDistance = 0xC,
		kLookAtSpellTargetHeightOffset = 0xD,
		kIsTurning = 0xE,
		kTurnAngleRemaining = 0xF,
		kMovementDirection = 0x10,
		kMovingTowardEnemy = 0x11,
		kPathingAngle = 0x12,
	};

	enum AnimationSyncGroupDataType
	{
		kTime = 0,
		kProgress = 1,
	};

	enum AnimationTrackBlendMode
	{
		kBlendTrack = 0,
		kAdditiveTrack = 1,
		kMultiAdditiveTrack = 2,
	};
}