#include <application.hpp>
#include <event_handler.hpp>
#include <event_handler/events.hpp>

#include <league_model/platform.hpp>
#include <league_model/bin_valuestorage.hpp>
#include <league_model/bin.hpp>
#include <league_model/skin.hpp>
#include <league_model/skeleton.hpp>
#include <league_model/animation.hpp>

#include <algorithm>

#include <profiling.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/compatibility.hpp>

#include <fstream>
uint32_t FNV1Hash(const std::string& a_String);

#if defined(__EMSCRIPTEN__)
void ReadyUp();
void Unready();
#endif

std::string GetStringByHash(uint32_t a_Hash);
Application* Application::Instance = nullptr;
double g_LastTime = 0;

Application::Application(const char* a_Root) :
	m_Window(Window::WindowSettings()),

	m_VertexShader(Shader::Type::Vertex),
	m_FragmentShader(Shader::Type::Fragment),

	m_Root(a_Root)
{
	Instance = this;

	if (m_Root[m_Root.size() - 1] != '/')
		m_Root += "/";
}

Application::~Application()
{
	m_Meshes.clear();
}

void Application::Init()
{
	Profiler::Context t(__FUNCTION__);

	EventHandler::Init();

	ADD_MEMBER_FUNCTION_EVENT_LISTENER(MouseDownEvent, Application, OnMouseDownEvent);
	ADD_MEMBER_FUNCTION_EVENT_LISTENER(MouseUpEvent, Application, OnMouseUp);
	ADD_MEMBER_FUNCTION_EVENT_LISTENER(PointerUpEvent, Application, OnPointerUpEvent);
	ADD_MEMBER_FUNCTION_EVENT_LISTENER(MouseScrollEvent, Application, OnMouseScrollEvent);
	ADD_MEMBER_FUNCTION_EVENT_LISTENER(PointerMoveEvent, Application, OnPointerMoveEvent);

	glClearColor(0.6f, 0.6f, 0.6f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_CULL_FACE);

	LoadDefaultTexture();
	LoadShaders();

#if defined(_WIN32)
	LoadSkin("data/output/data/characters/aatrox/skins/skin1.bin", "data/output/data/characters/aatrox/animations/skin1.bin");
#endif

	UpdateViewMatrix();

	Platform::SetMainLoop([]() 
	{
		// FileSystem::CloseLoadedFiles();

		double dt = Platform::GetTimeSinceStart() - g_LastTime;
		Profiler::Frame t("BottomLoop", dt);

		g_LastTime = Platform::GetTimeSinceStart();

		EventHandler::EmitEvent<UpdateEvent>(dt);
		EventHandler::Run();

		return Instance->Update(dt); 
	});

#if defined(__EMSCRIPTEN__)
	ReadyUp();
#endif
}

struct SkinLoadData
{
	SkinLoadData(const std::string& a_AnimationBinPath) : AnimationBinPath(a_AnimationBinPath) 
	{
#if defined(__EMSCRIPTEN__)
		Unready();
#endif
	}

	~SkinLoadData()
	{
#if defined(__EMSCRIPTEN__)
		ReadyUp();
#endif
	}

	std::string Texture;
	std::string AnimationBinPath;
	std::string AnimationName = "";

	bool AnimationLoaded = false;
	bool SkinLoaded = false;

	struct PrepareEventArgsT
	{
		PrepareEventArgsT(std::string& a_AnimationName, const League::BaseValueStorage* a_ParentStorage) :
			AnimationName(a_AnimationName),
			ParentStorage(a_ParentStorage)
		{}

		std::string AnimationName;
		const League::BaseValueStorage* ParentStorage;
	};
	std::vector<PrepareEventArgsT> PrepareEventArgs;

	Application::Mesh* Target = nullptr;
	League::Bin SkinBin;
	League::Bin AnimationBin;
	std::vector<League::Skin::Mesh> SubMeshes;
};

void PrepareEvents(const std::string& a_AnimationName, Application::Mesh& a_Mesh, std::vector<League::Skin::Mesh>& a_Submeshes, const League::BaseValueStorage* a_Parent)
{
	Profiler::Context t(__FUNCTION__);
	if (a_Parent == nullptr)
	{
		printf("No animation resource found.\n");
		return;
	}

	auto t_AnimationInfoStructBase = a_Parent->GetParent();
	if (t_AnimationInfoStructBase == nullptr)
	{
		printf("No animation info struct found.\n");
		return;
	}

	const auto& t_AnimationInfoStruct = *(const League::StructValueStorage*)t_AnimationInfoStructBase;
	auto t_EventMap = t_AnimationInfoStruct.GetChild("mEventDataMap");
	if (t_EventMap == nullptr) return;

	// Find all events by one of its members.
	auto t_EventMembers = t_EventMap->Find([](const League::BaseValueStorage& a_Value, void* a_UserData) { return a_Value.Is("mShowSubmeshList"); });
	if (t_EventMembers.size() == 0)
		return;

	//printf("Going through the event map for meshes to swap during the animation..\n");
	for (auto t_EventMember : t_EventMembers)
	{
		// This should be the mesh swap event
		auto t_Event = t_EventMember->GetParent();
		auto t_JSON = t_Event->GetAsJSON(false, false);

		const auto* t_MeshesToShowElement = (const League::ContainerValueStorage*)t_EventMember; // Already found, no need to GetChild this
		const auto* t_MeshesToHideElement = (const League::ContainerValueStorage*)t_Event->GetChild("mHideSubMeshList");
		const auto* t_FrameElement = (const League::NumberValueStorage<float>*)t_Event->GetChild("mStartFrame");
		if (t_FrameElement == nullptr || t_MeshesToShowElement == nullptr || t_MeshesToHideElement == nullptr) continue;

		float t_Frame = t_FrameElement->Get();
		std::vector<League::BaseValueStorage*> t_MeshesToShow = t_MeshesToShowElement->Get();
		std::vector<League::BaseValueStorage*> t_MeshesToHide = t_MeshesToHideElement->Get();

		printf("We're planning to show %lu meshes, and hide %lu meshes at frame %f during this animation.\n", t_MeshesToShow.size(), t_MeshesToHide.size(), t_Frame);

		std::vector<size_t> t_ToShow, t_ToHide;

		printf("Show:\n");
		for (auto& t_ShowMesh : t_MeshesToShow)
		{
			auto t_Storage = ((const League::HashValueStorage*)t_ShowMesh);
			auto t_MeshName = t_Storage->Get();
			printf("- %s: ", t_MeshName.c_str());
			bool t_Found = false;

			for (int i = 0; i < a_Submeshes.size(); i++)
			{
				if (a_Submeshes[i].Hash != t_Storage->GetData())
					continue;

				t_ToShow.push_back(i);
				t_Found = true;
				break;
			}

			printf("%s\n", t_Found ? "found" : "not found");
		}

		for (auto& t_HideMesh : t_MeshesToHide)
		{
			auto t_Storage = ((const League::HashValueStorage*)t_HideMesh);
			auto t_MeshName = t_Storage->Get();
			printf("- %s: ", t_MeshName.c_str());
			bool t_Found = false;

			for (int i = 0; i < a_Submeshes.size(); i++)
			{
				if (a_Submeshes[i].Hash != t_Storage->GetData())
					continue;

				t_ToHide.push_back(i);
				t_Found = true;
				break;
			}

			printf("%s\n", t_Found ? "found" : "not found");
		}

		a_Mesh.AnimationEvents[a_AnimationName].push_back(LM_NEW(ApplicationMesh::SwapMeshAnimationEvent(a_Mesh, t_Frame, t_ToShow, t_ToHide)));
	}
}

void OnSkinAndAnimationBin(SkinLoadData& a_LoadData)
{
	Profiler::Context t(__FUNCTION__);
	if (a_LoadData.AnimationLoaded == false || a_LoadData.SkinLoaded == false) return;
	
	if (a_LoadData.Target == nullptr)
	{
		printf("Callback for LoadSkin was called but target was unset, did we get an error?\n");
		LM_DEL(a_LoadData.Target);
		LM_DEL(&a_LoadData);
		return;
	}

	for (auto t_PrepareEventArg : a_LoadData.PrepareEventArgs)
		PrepareEvents(t_PrepareEventArg.AnimationName, *a_LoadData.Target, a_LoadData.SubMeshes, t_PrepareEventArg.ParentStorage);

	printf("Loaded both skin and animation information. Loading an animation: %s\n", a_LoadData.AnimationName.c_str());
	Application::Instance->LoadAnimation(*a_LoadData.Target, a_LoadData.AnimationName, [](League::Animation& a_Animation, void* a_UserData)
	{
		auto& t_LoadData = *(SkinLoadData*)a_UserData;

		if (a_Animation.GetLoadState() != File::LoadState::Loaded)
		{
			printf("Animation %s\n", a_Animation.GetLoadState() == File::LoadState::FailedToLoad ? "failed to load." : "was not found.");
			LM_DEL(&t_LoadData);
			return;
		}

		printf("Animation loaded! Using it.\n");
		t_LoadData.Target->AddAnimationReference(t_LoadData.AnimationName, a_Animation);
		t_LoadData.Target->ApplyAnimation(t_LoadData.AnimationName);
		LM_DEL(&t_LoadData);
	}, &a_LoadData);
}

void Application::LoadMesh(const std::string & a_SkinPath, const std::string & a_SkeletonPath)
{
	LoadMesh(a_SkinPath, a_SkeletonPath, [](const std::string& a_SkinPath, const std::string& a_SkeletonPath, Application::Mesh* a_Mesh, League::Skin& a_Skin, void* a_UserData) {});
}

void Application::LoadSkin(const std::string& a_BinPath, const std::string& a_AnimationBinPath)
{
	char t_ContextName[256];
	snprintf(t_ContextName, 256, "%s -> %s AND %s", __FUNCTION__, a_BinPath.c_str(), a_AnimationBinPath.c_str());
	Profiler::Context t(t_ContextName);
	auto* t_LoadData = LM_NEW(SkinLoadData(a_AnimationBinPath));

	// Load in the BIN containing most of the information about the base mesh
	t_LoadData->SkinBin.Load(a_BinPath, [](League::Bin& a_Bin, void* a_UserData)
	{
		Profiler::Context t("Application::LoadSkin->LoadSkinBin");

		auto* t_LoadData = (SkinLoadData*)a_UserData;
		if (a_Bin.GetLoadState() != File::LoadState::Loaded)
		{
			printf("Skin information bin %s!\n", a_Bin.GetLoadState() != File::LoadState::FailedToLoad ? "was not found" : "failed to load");

			t_LoadData->SkinLoaded = true;
			OnSkinAndAnimationBin(*t_LoadData); 
			return;
		}
		printf("Skin information is loaded!\n");

		auto t_MeshProperties = a_Bin.Get("skinMeshProperties");
		if (!t_MeshProperties)
		{
			printf("Mesh properties are missing..\n");

			t_LoadData->SkinLoaded = true;
			OnSkinAndAnimationBin(*t_LoadData);
			return;
		}

		printf("Mesh properties are valid!\n");

		const auto& t_Root = Application::Instance->GetAssetRoot();

		// Get the skeleton file
		auto t_SkeletonValue = (const League::StringValueStorage*)t_MeshProperties->GetChild("skeleton");
		if (!t_SkeletonValue)
		{
			t_LoadData->SkinLoaded = true;
			OnSkinAndAnimationBin(*t_LoadData);
			return;
		}
		std::string t_Skeleton = t_Root + t_SkeletonValue->Get();

		// Get the skin file
		auto t_SkinValue = (const League::StringValueStorage*)t_MeshProperties->GetChild("simpleSkin");
		if (!t_SkinValue)
		{
			t_LoadData->SkinLoaded = true;
			OnSkinAndAnimationBin(*t_LoadData);
			return;
		}
		std::string t_Skin = t_Root + t_SkinValue->Get();

		// Get the texture file
		auto t_TextureValue = (const League::StringValueStorage*)t_MeshProperties->GetChild("texture");
		if (t_TextureValue != nullptr)
			t_LoadData->Texture = t_Root + t_TextureValue->Get();
		else
		{
			const auto t_MaterialHash = (const League::HashValueStorage*)t_MeshProperties->GetChild("material");

			auto t_MaterialDefinition = t_LoadData->SkinBin.GetTopLevel(t_MaterialHash->GetData());
			for (auto& t_MatDefMember : *t_MaterialDefinition)
			{
				if (t_MatDefMember->GetType() != League::Bin::ValueStorage::Container)
					continue;

				const auto& t_Array = ((const League::ContainerValueStorage*)t_MatDefMember)->Get();
				for (int i = 0; i < t_Array.size(); i++)
				{
					const auto& t_Struct = ((const League::StructValueStorage*)t_Array[i]);
					auto t_Results = t_Struct->Find([](const League::BaseValueStorage& a_Value, void* a_UserData)
					{
						if (a_Value.GetType() != League::BaseValueStorage::Type::String)
							return false;

						auto t_Value = ((const League::StringValueStorage&)a_Value).Get();
						return t_Value == "Diffuse_Texture";
					});

					if (t_Results.size() == 0)
						continue;

					auto t_TextureStorage = t_Results[0]->GetParent()->GetChild("textureName");
					auto t_Texture = t_TextureStorage->DebugPrint();
					t_LoadData->Texture = t_Root + t_Texture;
					break;
				}
			}
		}

		printf("Starting to load the mesh..\n");
		
		// Load the mesh (Skeleton + Skin)
		Application::Instance->LoadMesh(t_Skin, t_Skeleton, [](const std::string& a_SkinPath, const std::string& a_SkeletonPath, Application::Mesh* a_Mesh, League::Skin& a_Skin, void* a_UserData)
		{
			Profiler::Context t("Application::LoadSkin->LoadMesh");
			auto* t_LoadData = (SkinLoadData*)a_UserData;
			if (a_Mesh == nullptr)
			{
				printf("Mesh did not load!\n");
				t_LoadData->SkinLoaded = true;
				OnSkinAndAnimationBin(*t_LoadData);
				return;
			}

			t_LoadData->Target = a_Mesh;

			printf("Mesh loaded!\n");
			const League::BaseValueStorage* t_MaterialOverrides = nullptr;
			auto t_InitialMeshStoragesToHide = t_LoadData->SkinBin.Find([](const League::BaseValueStorage& a_Value, void* a_UserData) { return a_Value.Is("initialSubmeshToHide"); });
			std::vector<uint32_t> t_InitialMeshesToHide;
			if (t_InitialMeshStoragesToHide.size() != 0)
			{
				for (auto t_Storage : t_InitialMeshStoragesToHide)
				{
					auto t_HashStorage = (const League::StringValueStorage*)t_Storage;
					auto t_String = t_HashStorage->Get();

					for (size_t i = 0; i < t_String.size(); )
					{
						size_t t_Index = t_String.find(' ', i);
						if (t_Index == std::string::npos) 
							t_Index = t_String.size();

						auto t_Mesh = t_String.substr(i, t_Index - i);
						t_InitialMeshesToHide.push_back(FNV1Hash(t_Mesh));

						i = t_Index + 1;
					}
				}
			}

			if (t_InitialMeshesToHide.size() != 0)
			{
				printf("Found %lu meshes to hide!\n", t_InitialMeshStoragesToHide.size());
				t_LoadData->SubMeshes = a_Skin.GetMeshes();
				for (size_t i = 0; i < t_LoadData->SubMeshes.size(); i++)
				{
					const auto& t_Submesh = t_LoadData->SubMeshes[i];
					bool t_ShouldHide = false;
					for (auto& t_SubmeshToHide : t_InitialMeshesToHide)
					{
						if (t_Submesh.Hash != t_SubmeshToHide)
							continue;

						t_ShouldHide = true;
						break;
					}

					printf("Hiding submesh '%s'.\n", t_Submesh.Name.c_str());
					a_Mesh->SubMeshes[i].Visible = !t_ShouldHide;
					a_Mesh->SubMeshes[i].InitialVisibility = !t_ShouldHide;
				}

				// Little hack to improve finding the materialOverride
				t_MaterialOverrides = t_InitialMeshStoragesToHide[0]->GetParent()->GetChild("materialOverride");
			}

			// Search for the material overrides, if needed
			if (t_MaterialOverrides == nullptr)
			{
				printf("Could not find it in the original place I suspected it would be, so retrying by searching everywhere..\n");
				auto t_Results = t_LoadData->SkinBin.Find([](const League::BaseValueStorage& a_Value, void* a_UserData) { return a_Value.Is("materialOverride"); });
				if (t_Results.size() != 0)
				{
					printf("There, found it.\n");
					t_MaterialOverrides = t_Results[0];
				}
				else printf("Could not find it.\n");
			}

			// Process material overrides
			std::vector<size_t> t_MeshesWithMaterial;
			if (t_MaterialOverrides != nullptr)
			{
				const auto& t_Root = Application::Instance->GetAssetRoot();
				auto t_Materials = (const League::ContainerValueStorage*)t_MaterialOverrides;
				for (const auto& t_Material : t_Materials->Get())
				{
					auto t_MaterialID = (const League::NumberValueStorage<unsigned int>*)t_Material->GetChild("material");
					auto t_TextureStorage = t_Material->GetChild("texture");
					auto t_SubmeshNameStorage = t_Material->GetChild("submesh");
					if ((t_MaterialID == nullptr && t_TextureStorage == nullptr) || t_SubmeshNameStorage == nullptr)
					{
						printf("Texture/Material and/or Submesh missing from struct of material override!\n");
						continue;
					}

					if (t_TextureStorage)
					{
						auto t_Texture = t_TextureStorage->DebugPrint();
						auto t_SubmeshName = t_SubmeshNameStorage->DebugPrint();
						for (int i = 0; i < t_LoadData->SubMeshes.size(); i++)
						{
							if (t_LoadData->SubMeshes[i].Name == t_SubmeshName)
							{
								printf("Submesh %s has its own texture: %s.\n", t_SubmeshName.c_str(), t_Texture.c_str());
								t_LoadData->Target->SubMeshes[i].SetTexture(t_Root + t_Texture);
								t_MeshesWithMaterial.push_back(i);
								break;
							}
						}
					}
					else
					{
						auto t_MaterialDefinition = t_LoadData->SkinBin.GetTopLevel(t_MaterialID->Get());
						if (t_MaterialDefinition == nullptr)
						{
							printf("Material ID was set as %u but I could not find it!\n", t_MaterialID->Get());
							continue;
						}
						
						for (auto& t_MatDefMember : *t_MaterialDefinition)
						{
							if (t_MatDefMember->GetType() != League::Bin::ValueStorage::Container)
								continue;

							const auto& t_Array = ((const League::ContainerValueStorage*)t_MatDefMember)->Get();
							for (int i = 0; i < t_Array.size(); i++)
							{
								const auto& t_Struct = ((const League::StructValueStorage*)t_Array[i]);
								auto t_Results = t_Struct->Find([](const League::BaseValueStorage& a_Value, void* a_UserData) 
								{ 
									if (a_Value.GetType() != League::BaseValueStorage::Type::String)
										return false;

									auto t_Value = ((const League::StringValueStorage&)a_Value).Get();
									return t_Value == "Diffuse_Texture"; 
								});

								if (t_Results.size() == 0)
									continue;

								bool t_Done = false;
								auto t_TextureStorage = t_Results[0]->GetParent()->GetChild("textureName");
								auto t_Texture = t_TextureStorage->DebugPrint();
								auto t_SubmeshName = t_SubmeshNameStorage->DebugPrint();
								for (int i = 0; i < t_LoadData->SubMeshes.size(); i++)
								{
									if (t_LoadData->SubMeshes[i].Name != t_SubmeshName)
										continue;

									printf("Submesh %s has its own texture: %s.\n", t_SubmeshName.c_str(), t_Texture.c_str());
									t_LoadData->Target->SubMeshes[i].SetTexture(t_Root + t_Texture);
									t_MeshesWithMaterial.push_back(i);
									t_Done = true;
									break;
								}

								if (t_Done) break;
							}
							break;
						}
					}
				}
			}

			printf("Setting other textures..\n");
			// Set the texture (async)
			for (int i = 0; i < a_Mesh->SubMeshes.size(); i++)
				if (std::find(t_MeshesWithMaterial.begin(), t_MeshesWithMaterial.end(), i) == t_MeshesWithMaterial.end())
					a_Mesh->SubMeshes[i].SetTexture(t_LoadData->Texture);

			t_LoadData->SkinLoaded = true;
			OnSkinAndAnimationBin(*t_LoadData);
		}, t_LoadData);
	}, t_LoadData);

	// Load all the animations
	t_LoadData->AnimationBin.Load(t_LoadData->AnimationBinPath, [](League::Bin& a_Bin, void* a_UserData)
	{
		Profiler::Context t("Application::LoadSkin->LoadAnimationBin");

		auto* t_LoadData = (SkinLoadData*)a_UserData;
		if (a_Bin.GetLoadState() != File::LoadState::Loaded)
		{
			t_LoadData->AnimationLoaded = true;
			OnSkinAndAnimationBin(*t_LoadData);
			return;
		}

		auto t_AnimationNames = a_Bin.Find([](const League::Bin::ValueStorage& a_ValueStorage, void* a_UserData)
		{
			if (a_ValueStorage.GetType() != League::Bin::ValueStorage::Type::String)
				return false;
			
			return a_ValueStorage.Is("mAnimationFilePath");
		});
		auto t_Name = t_AnimationNames.size() > 0 ? t_AnimationNames[0]->DebugPrint() : "";
		printf("Found all of the animations! We have %lu animations (%s).\n", t_AnimationNames.size(), t_Name.c_str());

		const auto& t_Root = Application::Instance->GetAssetRoot();

		const League::BaseValueStorage* t_Parent = nullptr;
		for (int i = 0; i < t_AnimationNames.size(); i++)
		{
			auto t_AnimationNameStorage = t_AnimationNames[i];
			const League::StringValueStorage* t_StringStorage = (League::StringValueStorage*)t_AnimationNameStorage;
			t_Parent = t_StringStorage->GetParent();

			auto t_AnimationName = t_Root + t_StringStorage->Get();

			// Try to load one of these..
			if (t_LoadData->AnimationName.size() == 0 &&
				((t_AnimationName.find("dance") != std::string::npos || t_AnimationName.find("Dance") != std::string::npos) ||
				(t_AnimationName.find("joke") != std::string::npos || t_AnimationName.find("Joke") != std::string::npos) ||
				(t_AnimationName.find("emote") != std::string::npos || t_AnimationName.find("Emote") != std::string::npos) ||
				(t_AnimationName.find("laugh") != std::string::npos || t_AnimationName.find("Laugh") != std::string::npos) ||
				(t_AnimationName.find("recall") != std::string::npos || t_AnimationName.find("Recall") != std::string::npos)))
				t_LoadData->AnimationName = t_AnimationName;

			if (t_LoadData->AnimationName.size() == 0 && i + 1 == t_AnimationNames.size())
			{
				printf("Could not find a suitable animation, so I gotta settle for %s..\nThe options were:\n", t_AnimationName.c_str());
				for (int i = 0; i < t_AnimationNames.size(); i++)
				{
					auto* t_StringStorage2 = (League::StringValueStorage*)t_AnimationNames[i];
					auto t_AnimationName2 = t_Root + t_StringStorage2->Get();
					printf(" - %s\n", t_AnimationName2.c_str());
				}

				t_LoadData->AnimationName = t_AnimationName;
			}

			Application::Instance->AddAnimationReference(t_AnimationName);
			t_LoadData->PrepareEventArgs.push_back(SkinLoadData::PrepareEventArgsT(t_AnimationName, t_Parent));
		}

		t_LoadData->AnimationLoaded = true;
		OnSkinAndAnimationBin(*t_LoadData);
	}, t_LoadData);
}

struct MeshLoadData
{
	MeshLoadData(const std::string& a_SkinPath, const std::string& a_SkeletonPath, Application::OnMeshLoadFunction a_Function, void* a_Argument) :
		SkinPath(a_SkinPath), SkeletonPath(a_SkeletonPath), OnLoadFunction(a_Function), Argument(a_Argument)
	{}

	bool SkinLoaded = false;
	bool SkeletonLoaded = false;

	std::string SkinPath;
	std::string SkeletonPath;
	League::Skin SkinTarget;
	League::Skeleton* SkeletonTarget;
	Application::OnMeshLoadFunction OnLoadFunction;
	void* Argument;
};

void Application::OnMeshLoad(MeshLoadData& a_LoadData)
{
	Profiler::Context t(__FUNCTION__);
	if (a_LoadData.SkinLoaded == false || a_LoadData.SkeletonLoaded == false)
		return;

	if (a_LoadData.SkinTarget.GetLoadState() != File::LoadState::Loaded)
	{
		if (a_LoadData.OnLoadFunction) a_LoadData.OnLoadFunction(a_LoadData.SkinPath, a_LoadData.SkeletonPath, nullptr, a_LoadData.SkinTarget, a_LoadData.Argument);
		LM_DEL(&a_LoadData);
		delete &a_LoadData.SkeletonTarget;
		return;
	}

	auto& t_Meshes = Application::Instance->m_Meshes;
	auto& t_ShaderProgram = Application::Instance->m_ShaderProgram;
	auto& t_Skin = a_LoadData.SkinTarget;

	// We can only start loading the mesh AFTER we load (or fail to load the skeleton), because the skeleton potentially modifies the bone indices
	Application::Mesh t_Mesh;
	if (a_LoadData.SkeletonTarget->GetLoadState() == File::LoadState::Loaded)
	{
		t_Mesh.Skeleton = std::make_shared<League::Skeleton>(*a_LoadData.SkeletonTarget); // Now the mesh owns this pointer
		t_Mesh.Skeleton->ApplyToSkin(a_LoadData.SkinTarget);
	}
	else
	{
		// Data is worthless to us, delete
		t_Mesh.Skeleton = nullptr;
		delete &a_LoadData.SkeletonTarget;
	}

	t_Mesh.PositionBuffer = t_ShaderProgram.GetVertexBuffer<glm::vec3>("v_Positions");
	t_Mesh.UVBuffer = t_ShaderProgram.GetVertexBuffer<glm::vec2>("v_UVs");
	t_Mesh.NormalBuffer = t_ShaderProgram.GetVertexBuffer<glm::vec3>("v_Normals");
	t_Mesh.BoneIndexBuffer = t_ShaderProgram.GetVertexBuffer<glm::vec4>("v_BoneIndices");
	t_Mesh.BoneWeightBuffer = t_ShaderProgram.GetVertexBuffer<glm::vec4>("v_BoneWeights");

	if (t_Mesh.PositionBuffer) t_Mesh.PositionBuffer->Upload(t_Skin.GetPositions());
	if (t_Mesh.UVBuffer) t_Mesh.UVBuffer->Upload(t_Skin.GetUVs());
	if (t_Mesh.NormalBuffer) t_Mesh.NormalBuffer->Upload(t_Skin.GetNormals());
	if (t_Mesh.BoneWeightBuffer) t_Mesh.BoneWeightBuffer->Upload(t_Skin.GetWeights());
	if (t_Mesh.BoneIndexBuffer) t_Mesh.BoneIndexBuffer->Upload(t_Skin.GetBoneIndices());

	auto& t_BoundingBox = t_Skin.GetBoundingBox();
	auto& t_SkinMeshes = t_Skin.GetMeshes();
	for (int i = 0; i < t_SkinMeshes.size(); i++)
	{
		Application::Mesh::SubMesh t_Submesh;

		t_Submesh.IndexBuffer = &t_ShaderProgram.GetIndexBuffer<uint16_t>();
		auto& t_SkinMesh = t_SkinMeshes[i];
		t_Submesh.IndexBuffer->Upload(t_SkinMesh.Indices, t_SkinMesh.IndexCount);

		t_Mesh.SubMeshes.push_back(t_Submesh);
	}

	std::transform(a_LoadData.SkinPath.begin(), a_LoadData.SkinPath.end(), a_LoadData.SkinPath.begin(), ::tolower);
	t_Meshes[a_LoadData.SkinPath] = t_Mesh;
	if (a_LoadData.OnLoadFunction) a_LoadData.OnLoadFunction(a_LoadData.SkinPath, a_LoadData.SkeletonPath, &t_Meshes[a_LoadData.SkinPath], a_LoadData.SkinTarget, a_LoadData.Argument);
	LM_DEL(&a_LoadData);
}

void Application::LoadMesh(const std::string& a_SkinPath, const std::string& a_SkeletonPath, OnMeshLoadFunction a_OnLoadFunction, void* a_UserData)
{
	Profiler::Context t(__FUNCTION__);
	m_Meshes.clear();
	m_AvailableAnimations.clear();

	auto* t_LoadData = LM_NEW(MeshLoadData(a_SkinPath, a_SkeletonPath, a_OnLoadFunction, a_UserData));

	t_LoadData->SkinTarget.Load(a_SkinPath, [](League::Skin& a_Skin, void* a_Argument)
	{
		Profiler::Context t("Application::LoadMesh->LoadSkin");
		auto& t_LoadData = *(MeshLoadData*)a_Argument;
		t_LoadData.SkinLoaded = true;
		Application::Instance->OnMeshLoad(t_LoadData);
	}, t_LoadData);

	t_LoadData->SkeletonTarget = new League::Skeleton();
	t_LoadData->SkeletonTarget->Load(a_SkeletonPath, [](League::Skeleton& a_Skeleton, void* a_Argument)
	{
		Profiler::Context t("Application::LoadMesh->LoadSkeleton");
		auto& t_LoadData = *(MeshLoadData*)a_Argument;

		t_LoadData.SkeletonLoaded = true;
		Application::Instance->OnMeshLoad(t_LoadData);
	}, t_LoadData);
}

void Application::LoadAnimation(Application::Mesh & a_Mesh, const std::string& a_AnimationPath, League::Animation::OnLoadFunction a_OnLoadFunction, void * a_UserData)
{
	Profiler::Context t(__FUNCTION__);

	if (a_Mesh.Skeleton == nullptr)
	{
		League::Animation t_FakeAnimation;
		if (a_OnLoadFunction) a_OnLoadFunction(t_FakeAnimation, a_UserData);
		return;
	}

	struct LoadData
	{
		LoadData(const std::string& a_AnimationPath, League::Animation::OnLoadFunction a_Function, void* a_Argument) :
			AnimationPath(a_AnimationPath), OnLoadFunction(a_Function), Argument(a_Argument)
		{}

		std::string AnimationPath;
		std::string SkeletonPath;
		League::Animation::OnLoadFunction OnLoadFunction;
		League::Animation Target;
		void* Argument;
	};
	auto* t_LoadData = LM_NEW(LoadData(a_AnimationPath, a_OnLoadFunction, a_UserData));

	t_LoadData->Target.Load(a_AnimationPath, [](League::Animation& a_Animation, void* a_Argument)
	{
		Profiler::Context t("Application::LoadAnimation->LoadAnimation");
		auto& t_LoadData = *(LoadData*)a_Argument;
		if (a_Animation.GetLoadState() != File::LoadState::Loaded)
		{
			if (t_LoadData.OnLoadFunction) t_LoadData.OnLoadFunction(a_Animation, t_LoadData.Argument);
			LM_DEL(&t_LoadData);
			return;
		}

		if (t_LoadData.OnLoadFunction) t_LoadData.OnLoadFunction(a_Animation, t_LoadData.Argument);
		LM_DEL(&t_LoadData);
	}, t_LoadData);
}

void Application::AddAnimationReference(const std::string & a_AnimationName)
{
	std::string t_Name = a_AnimationName;
	std::transform(t_Name.begin(), t_Name.end(), t_Name.begin(), ::tolower);
	printf("Adding reference: %s\n", t_Name.c_str());
	m_AvailableAnimations.push_back(t_Name);
}

const Texture & Application::GetDefaultTexture() const
{
	return m_DefaultTexture;
}

Application::Mesh * Application::GetMeshUnsafe(const std::string & a_Name)
{
	std::string t_Name = a_Name;
	std::transform(t_Name.begin(), t_Name.end(), t_Name.begin(), ::tolower);
	auto t_Index = m_Meshes.find(t_Name);
	if (t_Index == m_Meshes.end())
		return nullptr;

	return &t_Index->second;
}

const Application::Mesh * Application::GetMesh(const std::string & a_Name) const
{
	std::string t_Name = a_Name;
	std::transform(t_Name.begin(), t_Name.end(), t_Name.begin(), ::tolower);
	auto t_Index = m_Meshes.find(t_Name);
	if (t_Index == m_Meshes.end())
		return nullptr;

	return &t_Index->second;
}

const std::vector<std::string>& Application::GetAnimations() const
{
	return m_AvailableAnimations;
}

std::vector<std::string> Application::GetSkinFiles() const
{
	std::vector<std::string> t_Results;

	for (auto& t_MeshInfo : m_Meshes)
		t_Results.push_back(t_MeshInfo.first);

	return t_Results;
}

void Application::SetAssetRoot(const std::string& a_AssetRoot)
{
	m_Root = a_AssetRoot;
}

const std::string & Application::GetAssetRoot() const
{
	return m_Root;
}

void Application::OnMouseDownEvent(const MouseDownEvent * a_Event)
{
	if (a_Event->Button != Mouse::Button::Left) return;

	m_MouseIsDown = true;
	m_MousePosition = glm::vec2(a_Event->x, a_Event->y);
}

void Application::OnMouseUp(const MouseUpEvent * a_Event)
{
	if (a_Event->Button != Mouse::Button::Left) return;

	m_MouseIsDown = false;
}

void Application::OnPointerDownEvent(const PointerDownEvent * a_Event)
{
	if (a_Event->index != 0) return;

	m_MouseIsDown = true;
	m_MousePosition = glm::vec2(a_Event->x, a_Event->y);
}

void Application::OnPointerUpEvent(const PointerUpEvent * a_Event)
{
	if (a_Event->index != 0) return;

	m_MouseIsDown = false;
}

void Application::OnMouseScrollEvent(const MouseScrollEvent * a_Event)
{
	m_CameraDistance -= a_Event->Change;
	m_CameraDistance = glm::clamp(m_CameraDistance, 150.0f, 1000.0f);
	UpdateViewMatrix();
}

void Application::OnPointerMoveEvent(const PointerMoveEvent * a_Event)
{
	if (m_MouseIsDown == false) return;

	glm::vec2 t_CurrentMousePosition = glm::vec2(a_Event->x, a_Event->y);
	glm::vec2 t_RotationDelta = t_CurrentMousePosition - m_MousePosition;
	if (abs(t_RotationDelta.x) < 0.01f && abs(t_RotationDelta.y) < 0.01f) return;

	auto t_Rotation = glm::vec3(-t_RotationDelta.y, -t_RotationDelta.x, 0.0f) * 0.0174532925f;
	m_CameraPosition = glm::quat(t_Rotation) * m_CameraPosition;
	UpdateViewMatrix();

	m_MousePosition = t_CurrentMousePosition;
}

void Application::LoadDefaultTexture()
{
	Profiler::Context t(__FUNCTION__);
	m_DefaultTexture.Load("data/missing_texture.jpg");
}

void Application::LoadShaders()
{
	Profiler::Context t(__FUNCTION__);

	m_FragmentShader.Load("data/league_model.frag", [](Shader* a, void* b) { Instance->LoadShaderVariables(); });
	m_VertexShader.Load("data/league_model.vert", [](Shader* a, void* b) { Instance->LoadShaderVariables(); });
}

void Application::LoadShaderVariables()
{
	Profiler::Context t(__FUNCTION__);

	if (m_FragmentShader.GetLoadState() != File::LoadState::Loaded) return;
	if (m_VertexShader.GetLoadState() != File::LoadState::Loaded) return;

	m_ShaderProgram.Init({ m_FragmentShader, m_VertexShader });
	m_ShaderProgram.Use();

	m_MVPUniform = m_ShaderProgram.GetVariable<glm::mat4>("u_MVP");
	m_TextureUniform = m_ShaderProgram.GetVariable<Texture>("u_Diffuse");
	m_BoneArrayUniform = m_ShaderProgram.GetVariable<std::vector<glm::mat4>>("u_Bones");
}

void Application::UpdateViewMatrix()
{
	m_ViewMatrix = glm::lookAt(m_CameraPosition * m_CameraDistance, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
}

bool Application::Update(double a_DT)
{
	Profiler::Frame t(__FUNCTION__, a_DT);
#if defined(_WIN32)
	printf("%3.2f               \r", 1 / a_DT);
#endif

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	if (m_MVPUniform == nullptr)
	{
		Profiler::Context t2("WaitForSync");
		m_Window.SwapBuffers();
		return m_Window.RunFrame();
	}

	glViewport(0, 0, m_Window.GetWidth(), m_Window.GetHeight());
	m_ProjectionMatrix = glm::perspective(45.0f, (float)m_Window.GetWidth() / (float)m_Window.GetHeight(), 0.1f, 1500.0f);

	if (m_Meshes.size() == 0)
	{
		Profiler::Context t2("WaitForSync");
		m_Window.SwapBuffers();
		return m_Window.RunFrame();
	}

	for (auto& t_MeshInfo : m_Meshes)
	{
		auto& t_DrawMesh = t_MeshInfo.second;
		if (t_DrawMesh.SubMeshes.size() == 0)
		{
			Profiler::Context t2("WaitForSync");
			m_Window.SwapBuffers();
			return m_Window.RunFrame();
		}

		*m_MVPUniform = m_ProjectionMatrix * m_ViewMatrix * glm::translate(t_DrawMesh.Center);

		m_Time += a_DT;
		t_DrawMesh.Draw(m_Time, m_ShaderProgram, *m_MVPUniform, m_TextureUniform ? &m_TextureUniform->Get() : nullptr, m_BoneArrayUniform ? &m_BoneArrayUniform->Get() : nullptr);
	}

	{
		Profiler::Context t2("WaitForSync");
		m_Window.SwapBuffers();
		return m_Window.RunFrame();
	}
}

int main()
{
	Profiler::Get();
	printf("LeagueModel Application built on %s at %s\n", __DATE__, __TIME__);
	auto* t_Application = new Application("data/output");
	Application::Instance->Init();

#if defined(_WIN32)
	std::ofstream t_TraceFile("profile_result.json");
	t_TraceFile << Profiler::Get().GetJSON();
	t_TraceFile.close();
#endif

	std::atexit([]()
	{
		delete Application::Instance;
		Memory::Expose();
	});
}