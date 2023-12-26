#pragma once

#include "skin.hpp"
#include "skeleton.hpp"
#include "animation.hpp"
#include "animation_graph.hpp"
#include "character_animation.hpp"

#include <sokol_gfx.h>
#include <league_lib/bin/bin.hpp>

struct AnimatedMeshParametersVS_t;

namespace LeagueModel
{
	struct ManagedImage;

	enum CharacterLoadState : u64
	{
		NotLoaded			= 0b000000000000,
		InitFailed			= 0b000000000001,
		InfoLoadCompleted	= 0b000000000010,
		MeshGenCompleted	= 0b000000000100,
		CallbackCompleted	= 0b000000001000,
		SkinLoaded			= 0b000000010000,
		GraphLoaded			= 0b000000100000,
		SkeletonLoaded		= 0b000001000000,
		SkeletonApplied		= 0b000010000000,
		MaterialApplied		= 0b000100000000,
		SkinFailed			= 0b001000000000,
		GraphFailed			= 0b010000000000,
		SkeletonFailed		= 0b100000000000,

		FailedBitSet = InitFailed | SkinFailed | GraphFailed | SkeletonFailed,
		Loaded = InfoLoadCompleted | MeshGenCompleted | CallbackCompleted | SkinLoaded | GraphLoaded | SkeletonLoaded | SkeletonApplied | MaterialApplied,
	};

	struct SokolMesh
	{
		~SokolMesh();

		sg_bindings bindings = {};
		u32 indexCount = 0;
		bool shouldRender = false;
	};

	struct Character
	{
		~Character();

		using BoundingBox = Skin::BoundingBox;
		struct BoneFrameIndexCache
		{
			size_t translation = 0;
			size_t rotation = 0;
			size_t scale = 0;
		};

		LeagueModel::CharacterLoadState loadState;
		std::shared_ptr<ManagedImage> globalTexture;
		std::unordered_map<u32, std::shared_ptr<ManagedImage>> textures;

		LeagueLib::Bin info;
		Skin skin;
		Skeleton skeleton;
		BoundingBox box;
		AnimationGraph graph;
		f32 lastFrame = -1;
		f32 currentTime = 0;

		std::vector<SokolMesh> meshes;
		std::unordered_map<u32, SokolMesh*> meshMap;
		sg_pipeline pipeline;

		std::map<std::string, std::shared_ptr<Animation>> animations;
		std::unordered_map<std::string, AnimationClipBaseData*> clipMap;
		Animation* currentAnimation = nullptr;

		std::vector<BoneFrameIndexCache> currentFrameCache;
		glm::vec3 center = glm::vec3(0, 0, 0);
		u32 loadedSkinBinHash;

		using OnMeshLoadFunction = std::function<void(Character& character)>;
		void Load(const char* inModelName, u8 inSkinIndex, OnMeshLoadFunction inFunction = nullptr);
		void Reset();

		void PlayAnimation(const char* animationName);
		void PlayAnimation(const Animation& animation);

		void Update(AnimatedMeshParametersVS_t& args);

	private:
		Spek::File::Handle file;
	};
}
