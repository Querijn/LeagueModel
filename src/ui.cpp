#include "ui.hpp"
#include "character.hpp"

#include <string>
#include <cassert>
#include <thread>
#include <filesystem>
#include <fstream>
#include <imgui.h>
#include <json.hpp>

#if defined(_WIN32) || defined(_WIN64)
	#include <Windows.h>
	#include <wininet.h>
	#pragma comment(lib, "wininet.lib")
#elif defined(__EMSCRIPTEN__)
	#include <emscripten/fetch.h>
#endif

namespace LeagueModel
{
	namespace fs = std::filesystem;
	static struct
	{
		bool initialisationStarted = false;
		bool initialised = false;

		nlohmann::json champions;
		bool skinsWindowOpen = true;
		char skinsWindowFilter[256] = { 0 };

		bool animationWindowOpen = true;
		char animationWindowFilter[256] = { 0 };

		std::thread thread;
	} g_ui;

	static const fs::path cachePath = fs::current_path() / "cache";
	static const fs::path championCacheFile = cachePath / "champions.json";

	static void RequestChampionList()
	{
	#if defined(_WIN32) || defined(_WIN64)
		if (!fs::exists(cachePath))
			fs::create_directory(cachePath);

		if (fs::exists(championCacheFile))
		{
			std::ifstream file(championCacheFile);
			if (file.is_open())
			{
				std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
				g_ui.champions = nlohmann::json::parse(content);
				g_ui.initialised = true;
				return;
			}
		}

		g_ui.thread = std::thread([]()
		{
			std::string url = "https://cdn.merakianalytics.com/riot/lol/resources/latest/en-US/champions.json";

			HINTERNET client = InternetOpenA("LeagueModel", INTERNET_OPEN_TYPE_DIRECT, nullptr, nullptr, 0);
			if (client == nullptr)
			{
				assert(false);
				return;
			}

			HINTERNET	request = InternetOpenUrlA(client, url.c_str(), nullptr, 0, INTERNET_FLAG_RELOAD, 0);
			if (request == nullptr)
			{
				assert(false);
				InternetCloseHandle(client);
				return;
			}

			std::string responseBody;
			char		buffer[1024];
			DWORD		bytesRead;
			while (InternetReadFile(request, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0)
				responseBody.append(buffer, bytesRead);

			InternetCloseHandle(request);
			InternetCloseHandle(client);

			std::ofstream file(championCacheFile);
			if (file.is_open())
				file << responseBody;
			file.close();

			g_ui.champions = nlohmann::json::parse(responseBody);
			g_ui.initialised = true;
		});

		std::atexit([]()
		{
			if (g_ui.thread.joinable())
				g_ui.thread.join();
		});

	#elif defined(__EMSCRIPTEN__)
		std::string url = "https://cdn.merakianalytics.com/riot/lol/resources/latest/en-US/champions.json";

		emscripten_fetch_attr_t attr;
		emscripten_fetch_attr_init(&attr);
		strcpy(attr.requestMethod, "GET");
		attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
		attr.onsuccess = [](emscripten_fetch_t* fetch)
		{

			g_ui.champions = nlohmann::json::parse(fetch->data);
			g_ui.initialised = true;
		};

		attr.onerror = = [](emscripten_fetch_t* fetch)
		{
			printf("Downloading %s failed, HTTP failure status code: %d.\n", fetch->url, fetch->status);
		};

		emscripten_fetch(&attr, url.c_str());
	#endif
	}

	void RenderSkinsWindow(Character& character)
	{
		if (g_ui.skinsWindowOpen && ImGui::Begin("Skins", &g_ui.skinsWindowOpen))
		{
			ImGui::InputText("Filter", g_ui.skinsWindowFilter, sizeof(g_ui.skinsWindowFilter));
			if (ImGui::SmallButton("Clear Filter"))
				g_ui.skinsWindowFilter[0] = '\0';
			ImGui::Separator();

			for (auto& championKV : g_ui.champions.items())
			{
				const std::string& championName = championKV.key();
				const auto& champion = championKV.value();

				// Check if matches filter
				bool shouldFilter = false;
				if (g_ui.skinsWindowFilter[0] != '\0')
				{
					if (championName.find(g_ui.skinsWindowFilter) == std::string::npos)
						shouldFilter = true;

					for (auto& skinKV : champion["skins"].items())
					{
						auto& skin = skinKV.value();
						std::string skinName = skin["name"].get<std::string>() + " " + championName;
						if (skinName.find(g_ui.skinsWindowFilter) != std::string::npos)
						{
							shouldFilter = false;
							break;
						}
					}
				}

				if (shouldFilter)
					continue;

				if (ImGui::CollapsingHeader(championName.c_str()))
				{
					ImGui::Indent();
					for (auto& skinKV : champion["skins"].items())
					{
						auto& skin = skinKV.value();
						std::string skinName = skin["name"].get<std::string>() + " " + championName;
						if (g_ui.skinsWindowFilter[0] != '\0' && skinName.find(g_ui.skinsWindowFilter) == std::string::npos)
							continue;

						if (ImGui::Button(skinName.c_str()))
						{
							i32 championId = champion["id"].get<i32>();
							u8 skinId = skin["id"].get<i32>() - championId * 1000;
							character.Load(championName.c_str(), skinId);
						}
					}
					ImGui::Unindent();
				}
			}

			ImGui::End();
		}
	}

	void RenderAnimationsWindow(Character& character)
	{
		if (g_ui.skinsWindowOpen && ImGui::Begin("Animations", &g_ui.skinsWindowOpen))
		{
			ImGui::InputText("Filter", g_ui.animationWindowFilter, sizeof(g_ui.animationWindowFilter));
			if (ImGui::SmallButton("Clear Filter"))
				g_ui.animationWindowFilter[0] = '\0';
			ImGui::Separator();

			for (auto& animationKV : character.animations)
			{
				const std::string& animationName = animationKV.first;
				auto& animation = *animationKV.second;

				// Check if matches filter
				if (g_ui.animationWindowFilter[0] != '\0' && animationName.find(g_ui.animationWindowFilter) == std::string::npos)
					continue;

				const char* nameStart = strrchr(animationName.c_str(), '/') + 1;
				assert((u64)nameStart > 1);
				if (ImGui::Button(nameStart))
					character.PlayAnimation(animation);
			}

			ImGui::End();
		}
	}

	void RenderUI(Character& character)
	{
		if (!g_ui.initialised && !g_ui.initialisationStarted)
		{
			RequestChampionList();

			g_ui.initialisationStarted = true;
		}

		if (!g_ui.initialised)
			return;

		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Windows"))
			{
				ImGui::MenuItem("Skins", nullptr, &g_ui.skinsWindowOpen);
				ImGui::EndMenu();
			}

			ImGui::TextColored(ImVec4(1, 1, 1, 0.4), "FPS: %.1f", ImGui::GetIO().Framerate);
			ImGui::EndMainMenuBar();
		}

		RenderSkinsWindow(character);
		RenderAnimationsWindow(character);
	}
}
