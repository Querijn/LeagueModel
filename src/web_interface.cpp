#if defined(__EMSCRIPTEN__)

#include "application.hpp"

#include <profiling.hpp>

#include <emscripten/bind.h>
using namespace emscripten;

bool g_IsReady = false;

void LoadSkin(const std::string& a_SkinBin, std::string a_AnimationsBin)
{
	if (g_IsReady == false)
	{
		EM_ASM(console.error("Tried to load a skin while the application is not ready yet!"));
		return;
	}
	Application::Instance->LoadSkin(a_SkinBin, a_AnimationsBin);
}

void LoadMesh(const std::string& a_Skin, const std::string& a_Skeleton)
{
	Application::Instance->LoadMesh(a_Skin, a_Skeleton);
}

size_t GetAvailableAnimations(const std::string& a_Skin)
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
std::vector<std::string> GetSkinArray(const std::string& a_Skin)
{
	return Application::Instance->GetSkinFiles();
}

// TODO: Doesn't work? "Cannot call GetAnimationArray due to unbound types: NSt3__26vectorINS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEENS4_IS6_EEEE"
std::vector<std::string> GetAnimationArray(const std::string& a_Skin)
{
	return Application::Instance->GetAnimationsForMesh(a_Skin);
}

std::string GetAnimationName(const std::string& a_Skin, size_t a_Index)
{
	return Application::Instance->GetAnimationsForMesh(a_Skin)[a_Index];
}

void PlayAnimation(const std::string& a_Skin, std::string a_Animation)
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

std::string GetProfileResultsAsString()
{
	return Profiler::Get().GetJSON();
}

void GetProfileResults()
{
	EM_ASM
	(
		function download(content, fileName, contentType) 
		{
			var a = document.createElement("a");
			var file = new Blob([content], { type: contentType });
			a.href = URL.createObjectURL(file);
			a.download = fileName;
			a.click();
		}
		download(window["Module"]["GetProfileResultsAsString"](), 'profile_results.json', 'text/plain');
	);
}

void ReadyUp()
{
	g_IsReady = true;
	EM_ASM
	(
		if (Module.OnReady)
			Module.OnReady();
	);
	EM_ASM(console.log("LeagueModel is ready to go."));
}

void Unready()
{
	g_IsReady = false;
	EM_ASM(console.log("LeagueModel is unreadied."));
}

bool IsReady()
{
	return g_IsReady;
}

EMSCRIPTEN_BINDINGS(my_module) 
{
	function("IsReady", &IsReady);

	function("LoadSkin", &LoadSkin);
	function("LoadMesh", &LoadMesh);

	function("GetAvailableSkins", &GetAvailableSkins);
	function("GetSkinName", &GetSkinName);

	function("GetAnimationArray", &GetAnimationArray);
	function("GetAvailableAnimations", &GetAvailableAnimations);
	function("GetAnimationName", &GetAnimationName);
	function("PlayAnimation", &PlayAnimation);

	function("GetProfileResultsAsString", &GetProfileResultsAsString);
	function("GetProfileResults", &GetProfileResults);
}

#endif