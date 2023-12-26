#include "animation.hpp"
#include "character.hpp"
#include "managed_image.hpp"

#include "league_lib/bin/bin.hpp"
#include "league_lib/bin/bin_valuestorage.hpp"

#include <fnv1.hpp>
#include <cstring>
#include <ranges>
#include <string_view>
#include <algorithm>
#include <set>

#include "shaders/animated_mesh.hpp"

namespace LeagueModel
{
	using namespace Spek;
	static std::unordered_map<std::string, LeagueLib::Bin> g_bins;

	constexpr size_t g_shaderBoneCount = sizeof(decltype(AnimatedMeshParametersVS_t::bones)) / sizeof(decltype(AnimatedMeshParametersVS_t::bones[0]));

	void LoadAnimation(Character& character, const std::string& inAnimationPath, Animation::OnLoadFunction onAnimationLoaded);

	void CheckLoad(Character& character, Character::OnMeshLoadFunction onMeshLoaded, const LeagueLib::Bin& skinBin, const LeagueLib::BinVariable& base);
	void LoadMeshProperties(const LeagueLib::Bin& skinBin, const std::string& charName, u8 skinIndex, Character& character, Character::OnMeshLoadFunction onMeshLoaded, const LeagueLib::BinVariable& base);
	void TryLoadMeshInfo(Character& character, Character::OnMeshLoadFunction onMeshLoaded, const LeagueLib::Bin& skinBin, const LeagueLib::BinVariable& base);
	void TryGenerateMesh(Character& character, Character::OnMeshLoadFunction onMeshLoaded, const LeagueLib::Bin& skinBin);
	void FinishLoad(Character& character, Character::OnMeshLoadFunction onMeshLoaded);
	
	void Character::Load(const char* modelName, u8 skinIndex, OnMeshLoadFunction onMeshLoaded)
	{
		std::string charName = modelName;
		std::string skinBinFile = "data/characters/" + charName + "/skins/skin" + std::to_string(skinIndex) + ".bin";

		if (loadedSkinBinHash == FNV1Hash(skinBinFile))
			return;

		g_bins[skinBinFile].Load(skinBinFile.c_str(), [this, skinBinFile, charName, skinIndex, onMeshLoaded](const LeagueLib::Bin& skinBin)
		{
			if (skinBin.GetLoadState() != File::LoadState::Loaded)
			{
				loadState = CharacterLoadState::InitFailed;
				return;
			}

			Reset();
			loadedSkinBinHash = FNV1Hash(skinBinFile);

			const auto& base = skinBin["Characters/" + charName + "/Skins/Skin" + std::to_string(skinIndex)];
			LoadMeshProperties(skinBin, charName, skinIndex, *this, onMeshLoaded, base);

			// If it's a chroma, we have to use the parent's animation files
			i32 skinParent = base["skinParent"].IsValid() ? *base["skinParent"].As<i32>() : skinIndex;

			// Try to find the animation bin path
			std::string animBinPath;
			const u32* hash = base["skinAnimationProperties"]["animationGraphData"].As<u32>();
			for (auto& linkedFile : skinBin.GetLinkedFiles())
			{
				size_t extOffset = linkedFile.find_last_of(".bin");
				std::string fileCutOff = linkedFile.substr(strlen("DATA/"), extOffset - strlen("DATA/.bin") + 1);
				auto linkedHash = FNV1Hash(fileCutOff);
				if (linkedHash == *hash)
				{
					animBinPath = linkedFile;
					break;
				}
			}

			g_bins[animBinPath].Load(animBinPath, [&skinBin, this, skinParent, onMeshLoaded, charName, base](LeagueLib::Bin& animBin)
			{
				if (animBin.GetLoadState() != File::LoadState::Loaded)
				{
					(u64&)loadState |= (u64)CharacterLoadState::GraphFailed;
					CheckLoad(*this, onMeshLoaded, skinBin, base);
					return;
				}

				// Try to resolve the current root element
				i32 localSkinParent = skinParent;
				if (animBin["Characters/" + charName + "/Animations/Skin" + std::to_string(localSkinParent)].IsValid() == false)
				{
					std::string fileName = animBin.GetFileName();
					size_t nameOffset = strstr(fileName.c_str(), "/skin") - fileName.c_str() + strlen("/skin");
					size_t binOffset = strstr(fileName.c_str(), ".bin") - fileName.c_str();
					std::string skinIdString = fileName.substr(nameOffset, binOffset - nameOffset);

					try
					{
						localSkinParent = std::stoi(skinIdString);
					}
					catch (std::exception e)
					{
						printf("Failed to parse skin parent from animation file %s\n", fileName.c_str());
						(u64&)loadState |= (u64)CharacterLoadState::GraphFailed;
						CheckLoad(*this, onMeshLoaded, skinBin, base);
						return;
					}

					if (animBin["Characters/" + charName + "/Animations/Skin" + std::to_string(localSkinParent)].IsValid() == false)
					{
						printf("Failed to get animation graph root from animation file %s\n", fileName.c_str());
						(u64&)loadState |= (u64)CharacterLoadState::GraphFailed;
						CheckLoad(*this, onMeshLoaded, skinBin, base);
						return;
					}
				}

				// Here we setup the animation graph
				const auto& animBase = animBin["Characters/" + charName + "/Animations/Skin" + std::to_string(localSkinParent)];
				const auto* clipDataMap = animBase["mClipDataMap"].As<LeagueLib::BinMap>();
				const auto* maskDataMap = animBase["mMaskDataMap"].As<LeagueLib::BinMap>();
				const auto* trackDataMap = animBase["mTrackDataMap"].As<LeagueLib::BinMap>();

				// Create all clips, their masks and their tracks.
				for (auto& [hashVar, clip] : *clipDataMap)
					CreateClip(*hashVar.As<u32>(), clip, graph, clipDataMap, maskDataMap, trackDataMap);

				using AnimClipPair = std::pair<std::string, AnimationClipBaseData*>;
				std::set<AnimClipPair*> animations;
				for (auto& clipInfo : graph.clips)
				{
					if (clipInfo.second->Type == AnimationClipType::Atomic)
					{
						const AnimationAtomicClipData& clip = *(const AnimationAtomicClipData*)clipInfo.second.get();
						auto path = clip.mAnimationResourceData.mAnimationFilePath;

						if (path.empty() == false)
							animations.insert(new AnimClipPair{ clip.mAnimationResourceData.mAnimationFilePath, clipInfo.second.get() });
					}
				}

				for (auto& animationData : animations)
				{
					clipMap[animationData->first] = animationData->second;

					// Don't load every animation on web
				#if __EMSCRIPTEN__
					static bool plannedAnimationLoad = false;
					if (plannedAnimationLoad)
						continue;
					
					bool isLoop = !!strstr((char*)animationData->first.c_str(), "loop") ||
								  !!strstr((char*)animationData->first.c_str(), "Loop");
					if (!isLoop)
						continue;
					plannedAnimationLoad = true;
				#endif

					LoadAnimation(*this, animationData->first.c_str(), [animationData, &skinBin, onMeshLoaded, base, this](Animation& inAnimation)
					{
						bool isLoaded = inAnimation.loadState == File::LoadState::Loaded;
						if (isLoaded)
						{
							inAnimation.name = strrchr(animationData->first.c_str(), '/') + 1;
						}

						CheckLoad(*this, onMeshLoaded, skinBin, base);

						bool isLoop = !!strstr((char*)animationData->first.c_str(), "loop") ||
									  !!strstr((char*)animationData->first.c_str(), "Loop");
						if (isLoaded && currentAnimation == nullptr && isLoop)
							PlayAnimation(inAnimation);
					});
				}

				(u64&)loadState |= CharacterLoadState::GraphLoaded;
				CheckLoad(*this, onMeshLoaded, skinBin, base);
			});
		});
	}

	std::shared_ptr<ManagedImage> GetDiffuseTextureFromSamplerValues(Character& character, const LeagueLib::BinObject* material, ManagedImage::OnLoadFunction onLoad = nullptr)
	{
		if (material == nullptr)
			return nullptr;

		const LeagueLib::BinArray* samplerValues = material ? (*material)["samplerValues"].As<LeagueLib::BinArray>() : nullptr;
		if (samplerValues == nullptr)
		{
			printf("Sampler values missing from material override!\n");
			return nullptr;
		}

		for (const auto& samplerValue : *samplerValues)
		{
			const std::string* samplerName = samplerValue["samplerName"].As<std::string>();
			if (samplerName == nullptr)
				continue;

			bool isDiffuse = !!strstr(samplerName->c_str(), "Diffuse") || !!strstr(samplerName->c_str(), "diffuse");
			bool isTexture = !!strstr(samplerName->c_str(), "Texture") || !!strstr(samplerName->c_str(), "texture");
			if (isDiffuse && isTexture)
			{
				return std::make_shared<ManagedImage>(samplerValue["textureName"].As<std::string>()->c_str(), onLoad);
			}
		}

		return nullptr;
	}

	void LoadMeshProperties(const LeagueLib::Bin& skinBin, const std::string& charName, u8 skinIndex, Character& character, Character::OnMeshLoadFunction onMeshLoaded, const LeagueLib::BinVariable& base)
	{
		const auto& properties = base["skinMeshProperties"];
		const std::string* skeletonFileName = properties["skeleton"].As<std::string>();
		const std::string* skinFileName		= properties["simpleSkin"].As<std::string>();
		const std::string* textureFileName  = properties["texture"].As<std::string>();

		if (skeletonFileName == nullptr || skinFileName == nullptr)
		{
			if (skeletonFileName == nullptr) printf("The skeleton property is missing..\n");
			if (skinFileName == nullptr)	 printf("The skin property is missing..\n");
			(u64&)character.loadState |= CharacterLoadState::SkinFailed;
			CheckLoad(character, onMeshLoaded, skinBin, base);
			return;
		}

		if (textureFileName != nullptr)
		{
			character.globalTexture = std::make_shared<ManagedImage>(textureFileName->c_str());
		}
		else
		{
			character.globalTexture = nullptr;

			// TODO: Deduplicate this code
			const u32* materialHash = properties["material"].As<u32>();
			auto var = skinBin[*materialHash];
			const LeagueLib::BinArray* materialsArray = var.As<LeagueLib::BinArray>();
			const LeagueLib::BinObject* materialsObj = var.As<LeagueLib::BinObject>();
			if (materialsArray)
			{
				for (const auto& material : *materialsArray)
				{
					const std::string* textureFileName = material["Diffuse_Texture"].As<std::string>();
					if (textureFileName == nullptr)
						continue;

					character.globalTexture = std::make_shared<ManagedImage>(textureFileName->c_str());
					break;
				}
			}
			else if (materialsObj)
			{
				character.globalTexture = GetDiffuseTextureFromSamplerValues(character, materialsObj);
			}
		}

		bool hasOverriddenBBox = false;
		const glm::vec3* boxSizeOverride = properties["overrideBoundingBox"].As<glm::vec3>();
		if (boxSizeOverride != nullptr)
		{
			character.box.min = *boxSizeOverride + glm::vec3(-0.5, 0, -0.5); // TODO: This is not correct, make it good.
			character.box.max = *boxSizeOverride + glm::vec3(0.5, 1, 0.5);
			hasOverriddenBBox = true;
		}

		character.skin.Load(*skinFileName, [&character, &base, hasOverriddenBBox, onMeshLoaded, &skinBin](Skin& skin)
		{
			if (hasOverriddenBBox == false)
			{
				character.box = skin.boundingBox;
			}

			CheckLoad(character, onMeshLoaded, skinBin, base);
		});

		character.skeleton.Load(*skeletonFileName, [&character, &base, onMeshLoaded, &skinBin](Skeleton& skeleton)
		{
			CheckLoad(character, onMeshLoaded, skinBin, base);
		});
	}

	void CheckLoad(Character& character, Character::OnMeshLoadFunction onMeshLoaded, const LeagueLib::Bin& skinBin, const LeagueLib::BinVariable& base)
	{
		if ((character.loadState & CharacterLoadState::FailedBitSet) == 0) // Only try to load the meshes if we haven't failed yet
		{
			if ((character.loadState & CharacterLoadState::InfoLoadCompleted) == 0)
			{
				TryLoadMeshInfo(character, onMeshLoaded, skinBin, base);
				return;
			}

			if ((character.loadState & CharacterLoadState::MeshGenCompleted) == 0)
			{
				TryGenerateMesh(character, onMeshLoaded, skinBin);
				return;
			}
		}

		FinishLoad(character, onMeshLoaded);
	}

	void TryLoadMeshInfo(Character& character, Character::OnMeshLoadFunction onMeshLoaded, const LeagueLib::Bin& skinBin, const LeagueLib::BinVariable& base)
	{
		if (character.skin.loadState == File::LoadState::NotLoaded || character.skeleton.state == File::LoadState::NotLoaded)
			return;

		(u64&)character.loadState |= character.skin.loadState == File::LoadState::Loaded ? CharacterLoadState::SkinLoaded : CharacterLoadState::SkinFailed;
		(u64&)character.loadState |= character.skeleton.state == File::LoadState::Loaded ? CharacterLoadState::SkeletonLoaded : CharacterLoadState::SkeletonFailed;
		if (character.skin.loadState != File::LoadState::Loaded)
			return;

		auto checkLoaded = [&character, &base, onMeshLoaded, &skinBin](auto& _)
		{
			CheckLoad(character, onMeshLoaded, skinBin, base);
		};

		bool hasSkeletonApplied = (character.loadState & CharacterLoadState::SkeletonApplied) != 0;
		if (character.skeleton.state == File::LoadState::Loaded && !hasSkeletonApplied)
		{
			// Fix the bone indices on the skin
			for (auto& vertex : character.skin.vertices)
				for (int i = 0; i < 4; i++)
					vertex.boneIndices[i] = character.skeleton.boneIndices[vertex.boneIndices[i]];
			(u64&)character.loadState |= CharacterLoadState::SkeletonApplied;
		}

		bool hasMaterialsApplied = (character.loadState & CharacterLoadState::MaterialApplied) != 0;
		if (!hasMaterialsApplied)
		{
			(u64&)character.loadState |= CharacterLoadState::MaterialApplied;
			const auto& properties = base["skinMeshProperties"];
			const std::string* initialMeshToHide = properties["initialSubmeshToHide"].As<std::string>();
			std::vector<std::string> meshesToHide;
			if (initialMeshToHide != nullptr)
			{
				const char* start = initialMeshToHide->c_str();
				const char* end = start + initialMeshToHide->size();
				const char* current = start;
				for (;current != end; current++)
				{
					if (*current != ' ' && *current != ',')
						continue;

					meshesToHide.emplace_back(start, current);
					start = current + 1;
				}

				std::string last(start, current);
				if (last.empty() == false)
					meshesToHide.push_back(last);
			}

			for (auto& mesh : character.skin.meshes)
			{
				auto index = std::find(meshesToHide.begin(), meshesToHide.end(), mesh.name);
				mesh.initialVisibility = index == meshesToHide.end();
			}

			const LeagueLib::BinArray* materialOverride = properties["materialOverride"].As<LeagueLib::BinArray>();
			if (materialOverride)
			{
				for (const auto& material : *materialOverride)
				{
					const u32* materialHash = material["material"].As<u32>();
					const std::string* texture = material["texture"].As<std::string>();
					const std::string* submesh = material["submesh"].As<std::string>();
					if ((materialHash == nullptr && texture == nullptr) || submesh == nullptr)
					{
						printf("Texture/Material and/or submesh missing from struct of material override!\n");
						continue;
					}

					if (texture)
					{
						character.textures[FNV1Hash(*submesh)] = std::make_shared<ManagedImage>(texture->c_str(), checkLoaded);
					}

					const LeagueLib::BinObject* material = materialHash ? skinBin[*materialHash].As<LeagueLib::BinObject>() : nullptr;
					if (material)
					{
						// TODO: Load materials properly
						auto image = GetDiffuseTextureFromSamplerValues(character, material, checkLoaded);
						if (image)
							character.textures[FNV1Hash(*submesh)] = image;
					}
				}
			}
		}

		(u64&)character.loadState |= CharacterLoadState::InfoLoadCompleted;
		TryGenerateMesh(character, onMeshLoaded, skinBin);
	}

	void TryGenerateMesh(Character& character, Character::OnMeshLoadFunction onMeshLoaded, const LeagueLib::Bin& skinBin)
	{
		if (character.globalTexture != nullptr && character.globalTexture->loadState == File::LoadState::NotLoaded) // Waiting for global texture
			return;
		if ((character.loadState & CharacterLoadState::SkeletonApplied) == 0) // Waiting for skeleton to be applied
			return;

		for (auto& texture : character.textures)
		{
			File::LoadState loadState = texture.second->loadState;
			if (loadState == File::LoadState::NotLoaded)
				return;
		}

		auto& meshes = character.skin.meshes;
		sg_sampler_desc samplerDesc = {};
		samplerDesc.min_filter = SG_FILTER_LINEAR;
		samplerDesc.mag_filter = SG_FILTER_LINEAR;
		samplerDesc.mipmap_filter = SG_FILTER_LINEAR;
		samplerDesc.wrap_u = SG_WRAP_REPEAT;
		samplerDesc.wrap_v = SG_WRAP_REPEAT;
		sg_sampler sampler = sg_make_sampler(samplerDesc);

		sg_pipeline_desc pipelineDesc = {};
		pipelineDesc.layout.attrs[ATTR_AnimatedMeshVS_pos].format = SG_VERTEXFORMAT_FLOAT3;
		pipelineDesc.layout.attrs[ATTR_AnimatedMeshVS_uvs].format = SG_VERTEXFORMAT_FLOAT2;
		pipelineDesc.layout.attrs[ATTR_AnimatedMeshVS_normals].format = SG_VERTEXFORMAT_FLOAT3;
		pipelineDesc.layout.attrs[ATTR_AnimatedMeshVS_boneIndices].format = SG_VERTEXFORMAT_FLOAT4;
		pipelineDesc.layout.attrs[ATTR_AnimatedMeshVS_boneWeights].format = SG_VERTEXFORMAT_FLOAT4;

		pipelineDesc.shader = sg_make_shader(AnimatedMesh_shader_desc(sg_query_backend()));
		pipelineDesc.index_type = SG_INDEXTYPE_UINT16;
		pipelineDesc.cull_mode = SG_CULLMODE_FRONT;
		pipelineDesc.depth.compare = SG_COMPAREFUNC_LESS_EQUAL;
		pipelineDesc.depth.write_enabled = true;

		character.pipeline = sg_make_pipeline(pipelineDesc);

		for (const auto& sourceMesh : meshes)
		{
			sg_bindings bind = {};

			sg_buffer_desc indexBufferDesc = {};
			indexBufferDesc.type = SG_BUFFERTYPE_INDEXBUFFER;
			indexBufferDesc.data.size = sourceMesh.indexCount * sizeof(u16);
			indexBufferDesc.data.ptr = sourceMesh.indices;
			bind.index_buffer = sg_make_buffer(indexBufferDesc);

			if (character.textures.find(sourceMesh.hash) != character.textures.end())
				bind.fs.images[SLOT_diffuseTexture] = character.textures[sourceMesh.hash]->image;
			else
				bind.fs.images[SLOT_diffuseTexture] = character.globalTexture->image;
			bind.fs.samplers[SLOT_diffuseSampler] = sampler;

			sg_buffer_desc vertexBufferDesc = {};
			vertexBufferDesc.type = SG_BUFFERTYPE_VERTEXBUFFER;
			vertexBufferDesc.data.size = character.skin.vertices.size() * sizeof(Skin::Vertex);
			vertexBufferDesc.data.ptr = character.skin.vertices.data();
			bind.vertex_buffers[0] = sg_make_buffer(vertexBufferDesc);

			auto& mesh = character.meshes.emplace_back();
			mesh.bindings = bind;
			mesh.indexCount = sourceMesh.indexCount;
			mesh.shouldRender = sourceMesh.initialVisibility;
		}

		(u64&)character.loadState |= CharacterLoadState::MeshGenCompleted;
		FinishLoad(character, onMeshLoaded);
	}

	void LoadAnimation(Character& character, const std::string& inAnimationPath, Animation::OnLoadFunction inOnLoadFunction)
	{
		auto animation = std::make_shared<Animation>();
		animation->Load(inAnimationPath, [&character, inAnimationPath, animation, inOnLoadFunction](Animation& inAnimation)
		{
			if (inAnimation.loadState == File::LoadState::Loaded)
				character.animations[inAnimationPath] = animation;

			if (inOnLoadFunction)
				inOnLoadFunction(inAnimation);
		});
	}

	void FinishLoad(Character& character, Character::OnMeshLoadFunction onMeshLoaded)
	{
		if (((character.loadState & CharacterLoadState::SkinLoaded) == 0 && (character.loadState & CharacterLoadState::SkinFailed) == 0) ||
			((character.loadState & CharacterLoadState::SkeletonLoaded) == 0 && (character.loadState & CharacterLoadState::SkeletonFailed) == 0) ||
			((character.loadState & CharacterLoadState::GraphLoaded) == 0 && (character.loadState & CharacterLoadState::GraphFailed) == 0))
			return;

		if ((character.loadState & CharacterLoadState::CallbackCompleted) == 0)
		{
			(u64&)character.loadState |= CharacterLoadState::CallbackCompleted;
			if (onMeshLoaded)
				onMeshLoaded(character);
		}
	}

	void Character::Reset()
	{
		(u64&)loadState = 0;
		globalTexture = nullptr;
		textures.clear();
		loadedSkinBinHash = 0;

		info.Reset();
		skin = {};
		skeleton = {};
		box = {};
		graph = {};
		lastFrame = -1;
		currentTime = 0;

		meshes.clear();
		meshMap.clear();
		if (sg_isvalid())
			sg_destroy_pipeline(pipeline);

		animations.clear();
		clipMap.clear();
		currentAnimation = nullptr;

		currentFrameCache.clear();
		center = glm::vec3(0, 0, 0);
	}

	void Character::PlayAnimation(const char* inAnimationName)
	{
		const auto& index = animations.find(inAnimationName);
		if (index == animations.end())
		{
			printf("There was a request to play the animation %s, but I haven't got that one in my system!\n", inAnimationName);
			return;
		}

		currentAnimation = index->second.get();
		PlayAnimation(*currentAnimation);
	}

	void Character::PlayAnimation(const Animation& animation)
	{
		// TODO: Check if in map
		currentAnimation = const_cast<Animation*>(&animation);
		lastFrame = -1;
		currentTime = 0;
		currentFrameCache.clear();

		printf("Applying an animation with the following data:\n");
		printf("Name: %s\n", currentAnimation->name.c_str());
		printf("Duration: %f seconds\n", currentAnimation->duration);
		printf("FPS: %f\n", currentAnimation->fps);
		printf("Bone count: %llu\n", currentAnimation->bones.size());

		// We place the camera to be closer to the center of the character
		float highest(-9e9), lowest(9e9);
		size_t count = 0;
		for (auto& bone : currentAnimation->bones)
		{
			for (auto& trans : bone.translation)
			{
				if (trans.frameData.y > highest)
					highest = trans.frameData.y;
				else if (trans.frameData.y < lowest)
					lowest = trans.frameData.y;

				count++;
			}
		}
		center.y = -(highest + lowest) * 0.5f;
	}

	void ApplyFrame(Character& character, Animation* inAnimation, AnimationClipBaseData* inClip, f32 inCurrentFrame)
	{
		f32 frameDuration = inAnimation->duration * inAnimation->fps;

		for (auto& eventData : inClip->mEventDataMap)
		{
			switch (eventData.second->Type)
			{
			case AnimationEventType::SubmeshVisibilityEvent:
			{
				AnimationSubmeshVisibilityEventData& event = (AnimationSubmeshVisibilityEventData&)*eventData.second;
				assert(event.mStartFrame);

				f32 startFrame = event.mStartFrame;
				f32 endFrame = event.mEndFrame >= 0.0f ? event.mEndFrame : frameDuration;
				if (inCurrentFrame >= 0 && inCurrentFrame > startFrame && inCurrentFrame < endFrame)
					ApplyEvent(character, event, true);
				else if (inCurrentFrame >= endFrame || inCurrentFrame < 0.0f)
					ApplyEvent(character, event, false);
				break;
			}

			default:
				__debugbreak();
			}
		}
	}

	void Character::Update(AnimatedMeshParametersVS_t& args)
	{
		if (currentAnimation != nullptr && currentTime >= currentAnimation->duration)
			currentTime -= currentAnimation->duration;

		f32 currentFrame = currentAnimation ? currentTime * currentAnimation->fps : 0;
		auto clipIndex = currentAnimation ? clipMap.find(currentAnimation->name) : clipMap.end();
		if (clipIndex != clipMap.end())
		{
			if (lastFrame > currentFrame)
				ApplyFrame(*this, currentAnimation, clipIndex->second, -1);
			ApplyFrame(*this, currentAnimation, clipIndex->second, currentFrame);
		}

		glm::mat4 inverseRoot = glm::identity<glm::mat4>();

		if (currentFrameCache.size() < g_shaderBoneCount)
		{
			currentFrameCache.resize(g_shaderBoneCount);
			memset(currentFrameCache.data(), 0, currentFrameCache.size() * sizeof(BoneFrameIndexCache));
		}

		for (const auto& bone : skeleton.bones)
			if (bone.parent == nullptr)
				SetupHierarchy(*this, args.bones, bone, glm::identity<glm::mat4>());

		lastFrame = currentFrame; // TODO: Add resetting
	}

	Character::~Character()
	{
		Reset();
	}

	SokolMesh::~SokolMesh()
	{
		bindings = {};
	}
}
