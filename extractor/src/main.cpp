#include <wad_file.hpp>

#include <league_model/bin.hpp>
#include <league_model/bin_valuestorage.hpp>

#include <string>
#include <dirent.h>
#include <cassert>

#define CHAMPNAME_MAX 64

char g_Source[FILENAME_MAX] = { 0 };
char g_Output[FILENAME_MAX] = "../data/output/";
bool g_OutputMessages = false;

void HandleArguments(int argc, char* argv[])
{
	for (int i = 1; i < argc; i++)
	{
		if (strstr(argv[i], "-source=") == argv[i])
		{
			strncpy(g_Source, argv[i] + strlen("-source="), FILENAME_MAX);
			continue;
		}

		if (strstr(argv[i], "-output=") == argv[i])
		{
			strncpy(g_Output, argv[i] + strlen("-output="), FILENAME_MAX);
			continue;
		}

		if (strstr(argv[i], "-log=") == argv[i])
		{
			g_OutputMessages = strcmp(argv[i] + strlen("-log="), "true") == 0;
			continue;
		}

		else if (strstr(argv[i], "-log") == argv[i])
		{
			g_OutputMessages = true;
			continue;
		}
	}
}

void FixPath(char* a_Path)
{
	auto t_StringLength = strlen(a_Path);
	if (a_Path[t_StringLength - 1] == '/'
#if defined(_WIN32)
		|| a_Path[t_StringLength - 1] == '\\'
#endif
		)
		return;

	a_Path[t_StringLength] = '/';
	a_Path[t_StringLength + 1] = '\0';
}

void LowerCaseString(char* a_String)
{
	for (; *a_String; ++a_String) 
		*a_String = tolower(*a_String);
}

void ExtractFiles(WAD& a_WAD, const std::vector<std::string>& a_Files)
{
	char t_Destination[FILENAME_MAX];
	for (const auto& t_File : a_Files)
	{
		snprintf(t_Destination, FILENAME_MAX, "%s%s", g_Output, t_File.c_str());
		a_WAD.ExtractFile(t_File.c_str(), t_Destination);
	}
}

int main(int argc, char* argv[])
{
	HandleArguments(argc, argv);

	FixPath(g_Source);
	FixPath(g_Output);

	struct dirent *t_Entry;
	DIR *t_Directory = opendir(g_Source);
	if (t_Directory == NULL)
		return 1;

	char t_FileName[FILENAME_MAX] = { 0 };
	char t_ChampName[CHAMPNAME_MAX];
	while ((t_Entry = readdir(t_Directory)) != NULL)
	{	
		if (t_Entry->d_type == DT_DIR) continue;

		strncpy(t_FileName, g_Source, FILENAME_MAX);
		strncat(t_FileName, t_Entry->d_name, FILENAME_MAX);

		printf("Decompressing '%s'\n", t_Entry->d_name);
		auto t_WadFile = WAD(t_FileName);
		
		auto t_ChampNameLength = strchr(t_Entry->d_name, '.') - t_Entry->d_name;
		assert(t_ChampNameLength < CHAMPNAME_MAX && t_ChampNameLength > 0, "Champion name too long!");

		memcpy(t_ChampName, t_Entry->d_name, t_ChampNameLength);
		t_ChampName[t_ChampNameLength] = 0;
		LowerCaseString(t_ChampName);

		std::vector<std::string> t_FilesToGet;

		for (int i = 0; true; i++)
		{
			snprintf(t_FileName, FILENAME_MAX, "data/characters/%s/skins/skin%d.bin", t_ChampName, i);
			if (t_WadFile.HasFile(t_FileName))
				t_FilesToGet.push_back(t_FileName);
			else break;
		}
		
		ExtractFiles(t_WadFile, t_FilesToGet);

		for (auto t_File : t_FilesToGet)
		{
			if (g_OutputMessages) printf("Working on %s\n", t_File.c_str());

			League::Bin SkinBin;
			std::string t_Destination;
			SkinBin.Load(g_Output + t_File);
			if (SkinBin.GetLoadState() != File::LoadState::Loaded)
				throw 0;

			auto t_MeshProperties = SkinBin.Get("skinMeshProperties");
			if (!t_MeshProperties) throw 0;

			// Get the skeleton file
			auto t_SkeletonValue = (const League::StringValueStorage*)t_MeshProperties->GetChild("skeleton");
			if (!t_SkeletonValue) throw 0;
			std::string t_Skeleton = t_SkeletonValue->Get();
			t_Destination = g_Output + t_Skeleton;
			t_WadFile.ExtractFile(t_Skeleton.c_str(), t_Destination.c_str());
			if (g_OutputMessages) printf("Extracting skeleton '%s'\n", t_Skeleton.c_str());

			// Get the skin file
			auto t_SkinValue = (const League::StringValueStorage*)t_MeshProperties->GetChild("simpleSkin");
			if (!t_SkinValue) throw 0;
			std::string t_Skin = t_SkinValue->Get();
			t_Destination = g_Output + t_Skin;
			t_WadFile.ExtractFile(t_Skin.c_str(), t_Destination.c_str());
			if (g_OutputMessages) printf("Extracting skin '%s'\n", t_Skin.c_str());

			// Get the texture file
			auto t_TextureValue = (const League::StringValueStorage*)t_MeshProperties->GetChild("texture");
			std::string t_Texture;
			if (t_TextureValue == nullptr)
			{
				const auto t_MaterialHash = (const League::HashValueStorage*)t_MeshProperties->GetChild("material");
				if (t_MaterialHash != nullptr)
				{
					auto t_MaterialDefinition = SkinBin.GetTopLevel(t_MaterialHash->GetData());
					if (t_MaterialDefinition != nullptr)
					{
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
								t_Texture = t_TextureStorage->DebugPrint();
								t_Destination = g_Output + t_Texture;
								break;
							}
						}
					}
				}
			}
			else t_Texture = t_TextureValue->Get();

			if (t_Texture.size() != 0)
			{
				t_Destination = g_Output + t_Texture;
				t_WadFile.ExtractFile(t_Texture.c_str(), t_Destination.c_str());
				if (g_OutputMessages) printf("Extracting texture '%s'\n", t_Skin.c_str());
			}

			const League::BaseValueStorage* t_MaterialOverrides = nullptr;
			auto t_InitialMeshStoragesToHide = SkinBin.Find([](const League::BaseValueStorage& a_Value, void* a_UserData) { return a_Value.Is("initialSubmeshToHide"); });
			std::vector<uint32_t> t_InitialMeshesToHide;

			// Search for the material overrides, if needed
			if (t_MaterialOverrides == nullptr)
			{
				auto t_Results = SkinBin.Find([](const League::BaseValueStorage& a_Value, void* a_UserData) { return a_Value.Is("materialOverride"); });
				if (t_Results.size() != 0) t_MaterialOverrides = t_Results[0];
			}

			// Process material overrides
			std::vector<size_t> t_MeshesWithMaterial;
			if (t_MaterialOverrides != nullptr)
			{
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

						t_Destination = g_Output + t_Texture;
						t_WadFile.ExtractFile(t_Texture.c_str(), t_Destination.c_str());
					}
					else
					{
						auto t_MaterialDefinition = SkinBin.GetTopLevel(t_MaterialID->Get());
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

								auto t_TextureStorage = t_Results[0]->GetParent()->GetChild("textureName");
								auto t_Texture = t_TextureStorage->DebugPrint();
								auto t_SubmeshName = t_SubmeshNameStorage->DebugPrint();

								t_Destination = g_Output + t_Texture;
								t_WadFile.ExtractFile(t_Texture.c_str(), t_Destination.c_str());
								if (g_OutputMessages) printf("Extracting texture '%s'\n", t_Skin.c_str());
								break;
							}
							break;
						}
					}
				}
			}
		}

		t_FilesToGet.clear();

		for (int i = 0; true; i++)
		{
			snprintf(t_FileName, FILENAME_MAX, "data/characters/%s/animations/skin%d.bin", t_ChampName, i);
			if (t_WadFile.HasFile(t_FileName))
				t_FilesToGet.push_back(t_FileName);
			else break;
		}

		ExtractFiles(t_WadFile, t_FilesToGet);

		for (auto t_File : t_FilesToGet)
		{
			// Load all the animations
			League::Bin t_AnimationBin;
			t_AnimationBin.Load(g_Output + t_File);
			if (t_AnimationBin.GetLoadState() != File::LoadState::Loaded) throw 0;

			auto t_AnimationNames = t_AnimationBin.Find([](const League::Bin::ValueStorage& a_ValueStorage, void* a_UserData)
			{
				if (a_ValueStorage.GetType() != League::Bin::ValueStorage::Type::String)
					return false;

				return a_ValueStorage.Is("mAnimationFilePath");
			});

			for (int i = 0; i < t_AnimationNames.size(); i++)
			{
				auto t_AnimationNameStorage = t_AnimationNames[i];
				auto* t_StringStorage = (League::StringValueStorage*)t_AnimationNameStorage;

				auto t_AnimationName = t_StringStorage->Get();
				auto t_Destination = g_Output + t_AnimationName;
				t_WadFile.ExtractFile(t_AnimationName.c_str(), t_Destination.c_str());
			}
		}
	}

	closedir(t_Directory);

	return 0;
}