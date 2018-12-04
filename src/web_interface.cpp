#if defined(__EMSCRIPTEN__)

#include "application.hpp"

#include <emscripten/bind.h>
using namespace emscripten;

void LoadSkin(std::string a_SkinBin, std::string a_AnimationsBin)
{
	Application::Instance->LoadSkin(a_SkinBin, a_AnimationsBin);
}

size_t GetAvailableAnimations(std::string a_Skin)
{
	return Application::Instance->GetAnimationsForMesh(a_Skin).size();
}

size_t GetAvailableSkins()
{
	return Application::Instance->GetSkinFiles().size();
}

std::string GetSkinName(size_t a_Index)
{
	return Application::Instance->GetSkinFiles()[a_Index];
}

// TODO: Doesn't work? "Cannot call GetAnimationArray due to unbound types: NSt3__26vectorINS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEENS4_IS6_EEEE"
std::vector<std::string> GetSkinArray(std::string a_Skin)
{
	return Application::Instance->GetSkinFiles();
}

// TODO: Doesn't work? "Cannot call GetAnimationArray due to unbound types: NSt3__26vectorINS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEENS4_IS6_EEEE"
std::vector<std::string> GetAnimationArray(std::string a_Skin)
{
	return Application::Instance->GetAnimationsForMesh(a_Skin);
}

std::string GetAnimationName(std::string a_Skin, size_t a_Index)
{
	return Application::Instance->GetAnimationsForMesh(a_Skin)[a_Index];
}

void PlayAnimation(std::string a_Skin, std::string a_Animation)
{
	auto* t_Mesh = Application::Instance->GetMeshUnsafe(a_Skin);
	if (t_Mesh != nullptr)
	{
		printf("Could not play animation \"%s\", I don't know that animation for the mesh \"%s\"!\n", a_Animation.c_str(), a_Skin.c_str());
		return;
	}

	struct LoadData
	{
		LoadData(Application::Mesh* a_Skin, std::string a_AnimationName) : Target(a_Skin), AnimationName(a_AnimationName) {}

		std::string AnimationName;
		Application::Mesh* Target;
	};
	auto* t_LoadData = new LoadData(t_Mesh, a_Animation);

	Application::Instance->LoadAnimation(*t_Mesh, a_Animation, [](League::Animation& a_Animation, void* a_UserData)
	{
		auto* t_LoadData = (LoadData*)a_UserData;
		if (a_Animation.GetLoadState() != File::LoadState::Loaded)
		{
			return;
		}

		t_LoadData->Target->AddAnimationReference(t_LoadData->AnimationName, a_Animation);
		t_LoadData->Target->ApplyAnimation(t_LoadData->AnimationName);
	}, t_LoadData);
}

EMSCRIPTEN_BINDINGS(my_module) 
{
	function("LoadSkin", &LoadSkin); 

	function("GetAvailableSkins", &GetAvailableSkins);
	function("GetSkinName", &GetSkinName);

	function("GetAnimationArray", &GetAnimationArray);
	function("GetAvailableAnimations", &GetAvailableAnimations);
	function("GetAnimationName", &GetAnimationName);
	function("PlayAnimation", &PlayAnimation);
}

#endif