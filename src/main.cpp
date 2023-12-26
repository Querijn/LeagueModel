#include <sokol_wrapper.hpp>
#include <league_lib/wad/wad_filesystem.hpp>
#include <stb_image.h>
#include <filesystem>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/compatibility.hpp>

#include "types.hpp"
#include "character.hpp"
#include "simple_input.hpp"
#include "icon.hpp"
#include "ui.hpp"

#include "shaders/animated_mesh.hpp"

using namespace LeagueModel;
namespace fs = std::filesystem;
namespace LeagueModel
{
	namespace Input
	{
		void OnKeyDown(u32 virtualKey);
		void OnKeyUp(u32 virtualKey);
		void OnMouseDown(u32 button);
		void OnMouseUp(u32 button);
		void OnMouseScroll(f32 scrollDelta);
	}
}

static struct
{
	AnimatedMeshParametersVS_t parameters;
	sg_pass_action pass_action;

	glm::vec3 cameraPosition = glm::vec3(0, 0, 1);
	float cameraDistance = 400;
	glm::vec2 mousePosition;
	glm::mat4 viewMatrix;
	Character character;

	std::vector<const u8*> icons;
	ImGuiID dockSpaceID = 0;
} state;

static void Init()
{
	state.character.Load("aatrox", 0);

	sg_desc desc = {};
	desc.context = sapp_sgcontext();
	desc.logger.func = slog_func;
	sg_setup(&desc);

	if (!fs::exists("cache"))
		fs::create_directory("cache");

	simgui_desc_t simgui_desc = { };
	simgui_desc.ini_filename = "cache/imgui.ini";
	simgui_setup(&simgui_desc);
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowRounding = 0.0f;
	style.Colors[ImGuiCol_WindowBg].w = 1.0f;

	state.viewMatrix = glm::lookAt(state.cameraPosition * state.cameraDistance, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

	// default pass action
	state.pass_action.colors[0].load_action = SG_LOADACTION_CLEAR;
	state.pass_action.colors[0].clear_value = { 0.25f, 0.5f, 0.75f, 0.0f };
}

void RenderCharacter()
{
	int width = sapp_width();
	int height = sapp_height();
	state.character.Update(state.parameters);
	if ((state.character.loadState & CharacterLoadState::Loaded) != CharacterLoadState::Loaded)
		return;

	glm::mat4 proj = glm::perspective(45.0f, sapp_widthf() / sapp_heightf(), 1.0f, 500.0f + state.cameraDistance);
	glm::mat4 viewProj = proj * state.viewMatrix;
	glm::mat4 model = glm::translate(state.character.center);
	state.parameters.mvp = viewProj * model;

	sg_apply_pipeline(state.character.pipeline);
	sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_AnimatedMeshParametersVS, SG_RANGE(state.parameters));

	for (auto& m : state.character.meshes)
	{
		if (m.shouldRender == false)
			continue;

		sg_apply_bindings(&m.bindings);
		sg_draw(0, m.indexCount, 1);
	}
}

void OnFrame()
{
	auto scrollDelta = Input::ScrollDelta();
	if (scrollDelta != 0)
	{
		state.cameraDistance -= scrollDelta * 5.0f;
		state.cameraDistance = glm::clamp(state.cameraDistance, 150.0f, 1000.0f);
		state.viewMatrix = glm::lookAt(state.cameraPosition * state.cameraDistance, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
	}

	glm::vec2 currentMousePosition = glm::vec2(Input::GetMousePosition());
	if (Input::IsMouseButtonDown(Input::Left))
	{
		glm::vec2 rotDelta = currentMousePosition - state.mousePosition;
		if (abs(rotDelta.x) > 0.01f || abs(rotDelta.y) > 0.01f)
		{
			auto rot = glm::vec3(-rotDelta.y, -rotDelta.x, 0.0f) * 0.0174532925f;
			state.cameraPosition = glm::quat(rot) * state.cameraPosition;
			state.viewMatrix = glm::lookAt(state.cameraPosition * state.cameraDistance, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
		}
	}
	state.mousePosition = currentMousePosition;

	Input::Update();
	Spek::File::Update();

	int width = sapp_width();
	int height = sapp_height();
	state.character.currentTime += sapp_frame_duration();
	simgui_new_frame({ width, height, sapp_frame_duration(), sapp_dpi_scale() });
	sg_begin_default_pass(&state.pass_action, width, height);

	state.dockSpaceID = ImGui::DockSpaceOverViewport(nullptr, ImGuiDockNodeFlags_PassthruCentralNode);
	RenderCharacter();
	RenderUI(state.character);

	simgui_render();
	sg_end_pass();
	sg_commit();
}

void OnEvent(const sapp_event* inEvent)
{
	simgui_handle_event(inEvent);
	switch (inEvent->type)
	{
	case SAPP_EVENTTYPE_INVALID:
		return;

	case SAPP_EVENTTYPE_KEY_DOWN:
		if (ImGui::GetIO().WantCaptureKeyboard)
			break;
		Input::OnKeyDown((Input::KeyboardKey)inEvent->key_code);
		break;

	case SAPP_EVENTTYPE_KEY_UP:
		if (ImGui::GetIO().WantCaptureKeyboard)
			break;
		Input::OnKeyUp((Input::KeyboardKey)inEvent->key_code);
		break;

	case SAPP_EVENTTYPE_MOUSE_DOWN:
		if (ImGui::GetIO().WantCaptureMouse)
			break;
			Input::OnMouseDown((Input::MouseButton)inEvent->mouse_button);
		break;

	case SAPP_EVENTTYPE_MOUSE_UP:
		if (ImGui::GetIO().WantCaptureMouse)
			break;
		Input::OnMouseUp((Input::MouseButton)inEvent->mouse_button);
		break;

	case SAPP_EVENTTYPE_MOUSE_MOVE:
		if (ImGui::GetIO().WantCaptureMouse)
			break;
		const_cast<glm::ivec2&>(Input::GetMousePosition()) = glm::ivec2(inEvent->mouse_x, inEvent->mouse_y);
		break;

	case SAPP_EVENTTYPE_MOUSE_SCROLL:
		if (ImGui::GetIO().WantCaptureMouse)
			break;
		Input::OnMouseScroll(inEvent->scroll_y);
		break;

	case SAPP_EVENTTYPE_TOUCHES_BEGAN:
		if (ImGui::GetIO().WantCaptureMouse)
			break;

		for (int i = 0; i < inEvent->num_touches; i++)
		{
			auto& touch = inEvent->touches[i];
			if (touch.changed)
				continue;

			Input::OnMouseDown(0);
			break;
		}
		break;

	case SAPP_EVENTTYPE_TOUCHES_MOVED:
		if (ImGui::GetIO().WantCaptureMouse)
			break;

		for (int i = 0; i < inEvent->num_touches; i++)
		{
			auto& touch = inEvent->touches[i];
			if (!touch.changed)
				continue;

			const_cast<glm::ivec2&>(Input::GetMousePosition()) = glm::ivec2(touch.pos_x, touch.pos_y);
			break;
		}
		break;

	case SAPP_EVENTTYPE_TOUCHES_ENDED:
	case SAPP_EVENTTYPE_TOUCHES_CANCELLED:
		for (int i = 0; i < inEvent->num_touches; i++)
		{
			auto& touch = inEvent->touches[i];
			if (!touch.changed)
				continue;

			Input::OnMouseUp(0);
			break;
		}
		break;
	}
}


void CleanUp()
{
	state.character.Reset();
	sg_shutdown();

	for (auto& icon : state.icons)
		stbi_image_free((void*)icon);
}

void LoadIcon(sapp_image_desc& desc, const u8* data, size_t size)
{
	int bpp;
	const u8* imageData = stbi_load_from_memory(data, size, &desc.width, &desc.height, &bpp, 4);
	desc.pixels.ptr = imageData;
	desc.pixels.size = desc.width * desc.height * 4;
	state.icons.push_back(imageData);
}

void MountDir(const char* wadPath)
{
	for (auto& p : fs::recursive_directory_iterator(wadPath))
	{
		if (!p.is_regular_file())
			continue;

		if (p.path().filename().string() == "DATA.wad.client")
		{
			Spek::File::Mount<LeagueLib::WADFileSystem>(p.path().parent_path().string().c_str());
			break;
		}
	}
}

sapp_desc sokol_main(int argc, char* argv[])
{
	// Load current working directory and default or argument path
	MountDir(argc <= 1 ? "C:/Riot Games/League of Legends/Game/DATA/FINAL" : argv[1]);
	MountDir(fs::current_path().string().c_str());

	sapp_desc appDesc = {};
	appDesc.init_cb = Init;
	appDesc.frame_cb = OnFrame;
	appDesc.cleanup_cb = CleanUp;
	appDesc.event_cb = OnEvent;
	appDesc.width = 800;
	appDesc.height = 600;
	appDesc.high_dpi = true;
	appDesc.sample_count = 4;
	appDesc.ios_keyboard_resizes_canvas = true;
	appDesc.window_title = "LeagueModel";
	appDesc.logger.func = slog_func;

	appDesc.icon.sokol_default = false;
	LoadIcon(appDesc.icon.images[0], g_poroIcon1, sizeof(g_poroIcon1));
	LoadIcon(appDesc.icon.images[1], g_poroIcon2, sizeof(g_poroIcon2));
	LoadIcon(appDesc.icon.images[2], g_poroIcon3, sizeof(g_poroIcon3));

	return appDesc;
}