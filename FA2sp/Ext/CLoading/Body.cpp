#include "Body.h"

#include <CFinalSunApp.h>
#include <CMixFile.h>
#include <CINI.h>
#include "../../Helpers/Translations.h"
#include "../../FA2sp.h"
#include <filesystem>
#include "../CFinalSunApp/Body.h"

bool CLoadingExt::HasFile_ReadyToReadFromFolder = false;
Palette CLoadingExt::TempISOPalette = { 0 };
bool CLoadingExt::IsLoadingObjectView = false;
std::unordered_set<FString> CLoadingExt::SwimableInfantries;

bool CLoadingExt::InitMixFilesFix()
{
	HasMdFile = true;
	CLoadingExt::Ra2dotMixes.clear();

	// Load encrypted packages
	ResourcePackManager::instance().clear();
	if (auto pSection = CINI::FAData->GetSection("ExtraPackages"))
	{
		std::map<int, FString> collector;

		for (const auto& [key, index] : pSection->GetIndices())
			collector[index] = key;

		FString path;

		for (const auto& [_, key] : collector)
		{
			if (CINI::FAData->GetBool("ExtraPackages", key))
				path = CFinalSunApp::Instance->ExePath();
			else
				path = CFinalSunApp::Instance->FilePath();
			path += "\\" + key;

			if (ResourcePackManager::instance().loadPack(path))
			{
				Logger::Raw("[MixLoader][Package] %s loaded.\n", path);
			}
			else
			{
				Logger::Raw("[MixLoader][Package] %s failed!\n", path);
			}
		}
	}

	// Init Nested Mixes
	std::map<FString, std::vector<FString>> NestedMixes;
	if (auto pSection = CINI::FAData->GetSection("NestedMixes"))
	{
		std::map<int, FString> collector;

		for (const auto& [key, index] : pSection->GetIndices())
			collector[index] = key;

		for (const auto& [_, key] : collector)
		{
			auto value = pSection->GetString(key);
			auto name = FString::SplitString(value, "\\");
			name.back().MakeLower();
			NestedMixes[name.back()].push_back(key);
		}
	}

	if (!ExtConfigs::ExtMixLoader)
	{ 
		auto LoadNestedMix = [&NestedMixes](auto&& self, const FString& fileName, int parent, bool addToRa2 = false) -> void
		{
			auto name = FString::SplitString(fileName, "\\");
			name.back().MakeLower();

			for (const auto& file : NestedMixes[name.back()])
			{
				int result = CMixFile::Open(file, parent);
				if (result)
				{
					Logger::Raw("[MixLoader] %04d - %s loaded.\n", result, file);
					self(self, file, result, addToRa2);
					if (addToRa2)
					{
						CLoadingExt::Ra2dotMixes.insert(result);
					}
				}
				else
					Logger::Raw("[MixLoader] %s failed!\n", file);
			}
		};

		// Load Extra Mixes
		if (auto pSection = CINI::FAData->GetSection("ExtraMixes"))
		{
			std::map<int, FString> collector;

			for (const auto& [key, index] : pSection->GetIndices())
				collector[index] = key;

			FString path;

			for (const auto& [_, key] : collector)
			{
				if (CINI::FAData->GetBool("ExtraMixes", key))
					path = CFinalSunApp::Instance->ExePath();
				else
					path = CFinalSunApp::Instance->FilePath();
				path += "\\" + key;
				if (auto id = CMixFile::Open(path, 0))
				{
					Logger::Raw("[MixLoader][EXTRA] %04d - %s loaded.\n", id, path);
					LoadNestedMix(LoadNestedMix, key, id);
				}
				else
				{
					Logger::Raw("[MixLoader][EXTRA] %s failed!\n", path);
				}
			}
		}

		FString Dir = CFinalSunApp::Instance->FilePath();
		Dir += "\\";
		auto LoadMixFile = [this, Dir, &LoadNestedMix](const char* Mix, int Parent = 0, bool addToRA2 = false)
		{
			if (Parent)
			{
				int result = CMixFile::Open(Mix, Parent);
				if (result)
				{
					Logger::Raw("[MixLoader] %04d - %s loaded.\n", result, Mix);
					LoadNestedMix(LoadNestedMix, Mix, result, addToRA2);
					if (addToRA2)
					{
						CLoadingExt::Ra2dotMixes.insert(result);
					}
				}
				else
					Logger::Raw("[MixLoader] %s failed!\n", Mix);
				return ExtConfigs::DisableDirectoryCheck || result;
			}
			else
			{
				FString FullPath = Dir + Mix;
				int result = CMixFile::Open(FullPath, 0);
				if (result)
				{
					Logger::Raw("[MixLoader] %04d - %s loaded.\n", result, FullPath);
					LoadNestedMix(LoadNestedMix, Mix, result, addToRA2);
					if (addToRA2)
					{
						CLoadingExt::Ra2dotMixes.insert(result);
					}
					return ExtConfigs::DisableDirectoryCheck || result;
				}
				if (int nMix = SearchFile(Mix))
				{
					result = CMixFile::Open(Mix, nMix);
					if (result)
					{
						Logger::Raw("[MixLoader] %04d - %s loaded.\n", result, Mix);
						LoadNestedMix(LoadNestedMix, Mix, result, addToRA2);
						if (addToRA2)
						{
							CLoadingExt::Ra2dotMixes.insert(result);
						}
					}
					else
						Logger::Raw("[MixLoader] %s failed!\n", Mix);
					return ExtConfigs::DisableDirectoryCheck || result;
				}
				if (std::filesystem::exists(FullPath.c_str()))
				{
					Logger::Raw("[ExtMixLoader] %s failed!\n", Mix);
				}
				return ExtConfigs::DisableDirectoryCheck || result;
			}
		};
		auto SetMixFile = [LoadMixFile](const char* Mix, int& value)
		{
			value = LoadMixFile(Mix);
			return value;
		};

		FString fa2extra = CFinalSunApp::Instance->ExePath();
		fa2extra += "\\";
		fa2extra += "fa2extra.mix";
		if (auto id = CMixFile::Open(fa2extra, 0))
		{
			Logger::Raw("[MixLoader] %04d - %s loaded.\n", id, fa2extra);
			LoadNestedMix(LoadNestedMix, "fa2extra.mix", id);
		}
		else
		{
			Logger::Raw("[MixLoader] %s failed!\n", fa2extra);
		}

		FString format = "EXPAND" + CINI::FAData->GetString("Filenames", "MixExtension", "MD") + "%02d.MIX";
		for (int i = 99; i >= 0; --i)
		{
			FString filename;
			filename.Format(format, i);
			LoadMixFile(filename);
		}

		if (!LoadMixFile("RA2MD.MIX", 0, true))		return false;
		if (!LoadMixFile("RA2.MIX", 0, true))		return false;
		if (!LoadMixFile("CACHEMD.MIX", 0, true))	return false;
		if (!LoadMixFile("CACHE.MIX", 0, true))		return false;
		if (!LoadMixFile("LOCALMD.MIX", 0, true))	return false;
		if (!LoadMixFile("LOCAL.MIX", 0, true))		return false;

		// Init_Expansion_Mixfiles
		// ECACHE and ELOCAL
		WIN32_FIND_DATA fd;
		HANDLE hf = FindFirstFile(Dir + "ECACHE*.MIX", &fd);
		if (hf != INVALID_HANDLE_VALUE)
		{
			do
			{
				if (fd.dwFileAttributes & (FILE_ATTRIBUTE_TEMPORARY | FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN))
					continue;
				LoadMixFile(fd.cFileName);
			} while (FindNextFile(hf, &fd));
			FindClose(hf);
		}
		hf = FindFirstFile(Dir + "ELOCAL*.MIX", &fd);
		if (hf != INVALID_HANDLE_VALUE)
		{
			do
			{
				if (fd.dwFileAttributes & (FILE_ATTRIBUTE_TEMPORARY | FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN))
					continue;
				LoadMixFile(fd.cFileName);
			} while (FindNextFile(hf, &fd));
			FindClose(hf);
		}

		// Init_Secondary_Mixfiles
		if (!LoadMixFile("CONQMD.MIX", 0, true))		return false;
		if (!LoadMixFile("GENERMD.MIX", 0, true))	return false;
		if (!LoadMixFile("GENERIC.MIX", 0, true))	return false;
		if (!LoadMixFile("ISOGENMD.MIX", 0, true))	return false;
		if (!LoadMixFile("ISOGEN.MIX", 0, true))		return false;
		if (!LoadMixFile("CONQUER.MIX", 0, true))	return false;

		//MARBLE should be ahead of normal theater mixes
		FString FullPath = CFinalSunAppExt::ExePathExt;
		FullPath += "\\MARBLE.MIX";
		int result = CMixFile::Open(FullPath, 0);
		if (result)
		{
			Logger::Raw("[MixLoader] %04d - %s loaded.\n", result, FullPath);
			CFinalSunApp::Instance->MarbleLoaded = TRUE;
			LoadNestedMix(LoadNestedMix, "MARBLE.MIX", result);
		}
		else
		{
			if (LoadMixFile("MARBLE.MIX"))
				CFinalSunApp::Instance->MarbleLoaded = TRUE;
			else
			{
				CFinalSunApp::Instance->MarbleLoaded = FALSE;
				FString pMessage = Translations::TranslateOrDefault("MarbleMadnessNotLoaded",
					"Failed to load marble.mix! Framework mode won't be able to use!");
				::MessageBox(NULL, pMessage, Translations::TranslateOrDefault("Error", "Error"), MB_OK | MB_ICONEXCLAMATION);
			}
		}


		// Init_Theaters
		LoadMixFile("TEMPERATMD.MIX", 0, true);
		LoadMixFile("ISOTEMMD.MIX", 0, true);
		LoadMixFile("TEMPERAT.MIX", 0, true);
		LoadMixFile("ISOTEMP.MIX", 0, true);
		LoadMixFile("TEM.MIX", 0, true);

		LoadMixFile("SNOWMD.MIX", 0, true);
		LoadMixFile("ISOSNOMD.MIX", 0, true);
		LoadMixFile("SNOW.MIX", 0, true);
		LoadMixFile("ISOSNOW.MIX", 0, true);
		LoadMixFile("ISOSNO.MIX", 0, true);
		LoadMixFile("SNO.MIX", 0, true);

		LoadMixFile("URBANMD.MIX", 0, true);
		LoadMixFile("ISOURBMD.MIX", 0, true);
		LoadMixFile("URBAN.MIX", 0, true);
		LoadMixFile("ISOURB.MIX", 0, true);
		LoadMixFile("URB.MIX", 0, true);

		LoadMixFile("DESERT.MIX", 0, true);
		LoadMixFile("ISODES.MIX", 0, true);
		LoadMixFile("ISODESMD.MIX", 0, true);
		LoadMixFile("DES.MIX", 0, true);
		LoadMixFile("DESERTMD.MIX", 0, true);

		LoadMixFile("URBANNMD.MIX", 0, true);
		LoadMixFile("ISOUBNMD.MIX", 0, true);
		LoadMixFile("URBANN.MIX", 0, true);
		LoadMixFile("ISOUBN.MIX", 0, true);
		LoadMixFile("UBN.MIX", 0, true);

		LoadMixFile("LUNARMD.MIX", 0, true);
		LoadMixFile("ISOLUNMD.MIX", 0, true);
		LoadMixFile("LUNAR.MIX", 0, true);
		LoadMixFile("ISOLUN.MIX", 0, true);
		LoadMixFile("LUN.MIX", 0, true);

		LoadMixFile("LANGMD.MIX", 0, true);
		LoadMixFile("LANGUAGE.MIX", 0, true);
	}
	else
	{
		Logger::Raw("[ExtMixLoader] ExtMixLoader Enabled!\n");
		auto& manager = MixLoader::Instance();
		manager.Clear();

		auto LoadNestedMix = [&NestedMixes, &manager](auto&& self, const FString& fileName, int parent, bool addToRa2 = false) -> void
		{
			auto name = FString::SplitString(fileName, "\\");
			name.back().MakeLower();

			for (const auto& file : NestedMixes[name.back()])
			{
				int result = manager.LoadMixFile(file, parent);
				if (result)
				{
					Logger::Raw("[ExtMixLoader] %04d - %s loaded.\n", result, file);
					self(self, file, result, addToRa2);
					if (addToRa2)
					{
						CLoadingExt::Ra2dotMixes.insert(result);
					}
				}
				else
					Logger::Raw("[ExtMixLoader] %s failed!\n", file);
			}
		};

		// Load Extra Mixes
		if (auto pSection = CINI::FAData->GetSection("ExtraMixes"))
		{
			std::map<int, FString> collector;

			for (const auto& [key, index] : pSection->GetIndices())
				collector[index] = key;

			FString path;

			for (const auto& [_, key] : collector)
			{
				if (CINI::FAData->GetBool("ExtraMixes", key))
					path = CFinalSunApp::Instance->ExePath();
				else
					path = CFinalSunApp::Instance->FilePath();
				path += "\\" + key;
				if (auto id = manager.LoadMixFile(path))
				{
					Logger::Raw("[ExtMixLoader][EXTRA] %04d - %s loaded.\n", id, path);
					LoadNestedMix(LoadNestedMix, key, id);
				}
				else
				{
					Logger::Raw("[ExtMixLoader][EXTRA] %s failed!\n", path);
				}
			}
		}

		FString Dir = CFinalSunApp::Instance->FilePath();
		Dir += "\\";
		auto LoadMixFile = [&manager, this, Dir, &LoadNestedMix](const char* Mix, int Parent = 0, bool addToRA2 = false)
		{
			FString FullPath = Dir + Mix;
			int parent = -1;
			auto id = manager.LoadMixFile(FullPath, &parent);
			if (id)
			{
				if (parent >= 0)
					Logger::Raw("[ExtMixLoader] %04d - %s loaded.\n", id, Mix);
				else
					Logger::Raw("[ExtMixLoader] %04d - %s loaded.\n", id, FullPath);
				LoadNestedMix(LoadNestedMix, Mix, id, addToRA2);
				if (addToRA2)
				{
					CLoadingExt::Ra2dotMixes.insert(id);
				}
				return ExtConfigs::DisableDirectoryCheck || id;
			}
			if (std::filesystem::exists(FullPath.c_str()))
			{
				Logger::Raw("[ExtMixLoader] %s failed!\n", Mix);
			}
			return ExtConfigs::DisableDirectoryCheck || id;
		};

		FString fa2extra = CFinalSunApp::Instance->ExePath();
		fa2extra += "\\";
		fa2extra += "fa2extra.mix";
		if (auto id = manager.LoadMixFile(fa2extra))
		{
			Logger::Raw("[ExtMixLoader] %04d - %s loaded.\n", id, fa2extra);
			LoadNestedMix(LoadNestedMix, "fa2extra.mix", id);
		}
		else
		{
			Logger::Raw("[ExtMixLoader] %s failed!\n", fa2extra);
		}

		FString format = "EXPAND" + CINI::FAData->GetString("Filenames", "MixExtension", "MD") + "%02d.MIX";
		for (int i = 99; i >= 0; --i)
		{
			FString filename;
			filename.Format(format, i);
			LoadMixFile(filename);
		}

		if (!LoadMixFile("RA2MD.MIX", 0, true))		return false;
		if (!LoadMixFile("RA2.MIX", 0, true))		return false;
		if (!LoadMixFile("CACHEMD.MIX", 0, true))	return false;
		if (!LoadMixFile("CACHE.MIX", 0, true))		return false;
		if (!LoadMixFile("LOCALMD.MIX", 0, true))	return false;
		if (!LoadMixFile("LOCAL.MIX", 0, true))		return false;

		// Init_Expansion_Mixfiles
		// ECACHE and ELOCAL
		WIN32_FIND_DATA fd;
		HANDLE hf = FindFirstFile(Dir + "ECACHE*.MIX", &fd);
		if (hf != INVALID_HANDLE_VALUE)
		{
			do
			{
				if (fd.dwFileAttributes & (FILE_ATTRIBUTE_TEMPORARY | FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN))
					continue;
				LoadMixFile(fd.cFileName);
			} while (FindNextFile(hf, &fd));
			FindClose(hf);
		}
		hf = FindFirstFile(Dir + "ELOCAL*.MIX", &fd);
		if (hf != INVALID_HANDLE_VALUE)
		{
			do
			{
				if (fd.dwFileAttributes & (FILE_ATTRIBUTE_TEMPORARY | FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN))
					continue;
				LoadMixFile(fd.cFileName);
			} while (FindNextFile(hf, &fd));
			FindClose(hf);
		}

		// Init_Secondary_Mixfiles
		if (!LoadMixFile("CONQMD.MIX", 0, true))		return false;
		if (!LoadMixFile("GENERMD.MIX", 0, true))	return false;
		if (!LoadMixFile("GENERIC.MIX", 0, true))	return false;
		if (!LoadMixFile("ISOGENMD.MIX", 0, true))	return false;
		if (!LoadMixFile("ISOGEN.MIX", 0, true))		return false;
		if (!LoadMixFile("CONQUER.MIX", 0, true))	return false;

		//MARBLE should be ahead of normal theater mixes
		FString FullPath = CFinalSunAppExt::ExePathExt();
		FullPath += "\\MARBLE.MIX";
		int id = manager.LoadMixFile(FullPath);
		if (id)
		{
			Logger::Raw("[ExtMixLoader] %04d - %s loaded.\n", id, FullPath);
			CFinalSunApp::Instance->MarbleLoaded = TRUE;
			LoadNestedMix(LoadNestedMix, "MARBLE.MIX", id);
		}
		else
		{
			if (LoadMixFile("MARBLE.MIX"))
				CFinalSunApp::Instance->MarbleLoaded = TRUE;
			else
			{
				CFinalSunApp::Instance->MarbleLoaded = FALSE;
				FString pMessage = Translations::TranslateOrDefault("MarbleMadnessNotLoaded",
					"Failed to load marble.mix! Framework mode won't be able to use!");
				::MessageBox(NULL, pMessage, Translations::TranslateOrDefault("Error", "Error"), MB_OK | MB_ICONEXCLAMATION);
			}
		}


		// Init_Theaters
		LoadMixFile("TEMPERATMD.MIX", 0, true);
		LoadMixFile("ISOTEMMD.MIX", 0, true);
		LoadMixFile("TEMPERAT.MIX", 0, true);
		LoadMixFile("ISOTEMP.MIX", 0, true);
		LoadMixFile("TEM.MIX", 0, true);

		LoadMixFile("SNOWMD.MIX", 0, true);
		LoadMixFile("ISOSNOMD.MIX", 0, true);
		LoadMixFile("SNOW.MIX", 0, true);
		LoadMixFile("ISOSNOW.MIX", 0, true);
		LoadMixFile("ISOSNO.MIX", 0, true);
		LoadMixFile("SNO.MIX", 0, true);

		LoadMixFile("URBANMD.MIX", 0, true);
		LoadMixFile("ISOURBMD.MIX", 0, true);
		LoadMixFile("URBAN.MIX", 0, true);
		LoadMixFile("ISOURB.MIX", 0, true);
		LoadMixFile("URB.MIX", 0, true);

		LoadMixFile("DESERT.MIX", 0, true);
		LoadMixFile("ISODES.MIX", 0, true);
		LoadMixFile("ISODESMD.MIX", 0, true);
		LoadMixFile("DES.MIX", 0, true);
		LoadMixFile("DESERTMD.MIX", 0, true);

		LoadMixFile("URBANNMD.MIX", 0, true);
		LoadMixFile("ISOUBNMD.MIX", 0, true);
		LoadMixFile("URBANN.MIX", 0, true);
		LoadMixFile("ISOUBN.MIX", 0, true);
		LoadMixFile("UBN.MIX", 0, true);

		LoadMixFile("LUNARMD.MIX", 0, true);
		LoadMixFile("ISOLUNMD.MIX", 0, true);
		LoadMixFile("LUNAR.MIX", 0, true);
		LoadMixFile("ISOLUN.MIX", 0, true);
		LoadMixFile("LUN.MIX", 0, true);

		LoadMixFile("LANGMD.MIX", 0, true);
		LoadMixFile("LANGUAGE.MIX", 0, true);
	}

	return true;
}