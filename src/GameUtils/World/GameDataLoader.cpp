#include "EngineCommon/precomp.h"

#include "GameUtils/World/GameDataLoader.h"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>

#include <nlohmann/json.hpp>

#include "EngineCommon/Random/RandomStrings.h"

#include "GameData/GameData.h"
#include "GameData/WorldLayer.h"

namespace GameDataLoader
{
	static const std::filesystem::path MAPS_PATH = "./resources/maps";
	static const std::filesystem::path GAME_DATA_PATH = "./resources/game";

	void SaveWorld(WorldLayer& world, const std::filesystem::path& workingPath, const std::string& levelName, const Json::ComponentSerializationHolder& jsonSerializerHolder)
	{
		SCOPED_PROFILER("SaveWorld");

		namespace fs = std::filesystem;
		fs::path levelPath(levelName);

		// if it's name, we save to maps folder
		if (levelName.find_first_of("/\\.") == std::string::npos)
		{
			levelPath = workingPath / MAPS_PATH / (levelName + ".json");
		}
		else
		{
			if (!levelPath.has_extension())
			{
				levelPath = levelPath.string() + ".json";
			}
		}

		std::string levelVersion = std::string("d_").append(StringUtils::getRandomWordSafeBase32(10));

		try
		{
			std::ofstream mapFile(levelPath);
			nlohmann::json mapJson({
				{"version", levelVersion},
				{"world", world.toJson(jsonSerializerHolder)}
			});

			mapFile << std::setw(4) << mapJson << std::endl;
		}
		catch (const std::exception& e)
		{
			ReportError("Can't save world to file '%s': %s", levelPath.c_str(), e.what());
		}
	}

	void LoadWorld(WorldLayer& world, const std::filesystem::path& workingPath, const std::string& levelName, const Json::ComponentSerializationHolder& jsonSerializerHolder)
	{
		SCOPED_PROFILER("LoadWorld");

		namespace fs = std::filesystem;
		fs::path levelPath(levelName);

		// if it is a name, we search the map in the maps folder
		if (levelName.find_first_of("/\\.") == std::string::npos)
		{
			levelPath = workingPath / MAPS_PATH / (levelName + ".json");
		}

		try
		{
			std::ifstream mapFile(levelPath);
			nlohmann::json mapJson;
			mapFile >> mapJson;

			const std::string levelVersion = mapJson.contains("version") ? mapJson.at("version") : "";

			if (const auto& worldObject = mapJson.at("world"); worldObject.is_object())
			{
				world.fromJson(worldObject, jsonSerializerHolder);
			}
		}
		catch (const nlohmann::detail::exception& e)
		{
			ReportError("Can't parse world '%s': %s", levelPath.c_str(), e.what());
		}
		catch (const std::exception& e)
		{
			ReportError("Can't open world '%s': %s", levelPath.c_str(), e.what());
		}
	}

	void SaveGameData(const GameData& gameData, const std::filesystem::path& workingPath, const std::string& gameDataName, const Json::ComponentSerializationHolder& jsonSerializerHolder)
	{
		SCOPED_PROFILER("SaveGameData");

		namespace fs = std::filesystem;
		fs::path gameDataPath(gameDataName);

		// if it's name, we save to maps folder
		if (gameDataName.find_first_of("/\\.") == std::string::npos)
		{
			gameDataPath = workingPath / GAME_DATA_PATH / (gameDataName + ".json");
		}
		else
		{
			if (!gameDataPath.has_extension())
			{
				gameDataPath = gameDataPath.string() + ".json";
			}
		}

		try
		{
			std::ofstream mapFile(gameDataPath);
			nlohmann::json mapJson({ { "gameData", gameData.toJson(jsonSerializerHolder) } });

			mapFile << std::setw(4) << mapJson << std::endl;
		}
		catch (const std::exception& e)
		{
			ReportError("Can't save gameData to file '%s': %s", gameDataPath.c_str(), e.what());
		}
	}

	void LoadGameData(GameData& gameData, const std::filesystem::path& workingPath, const std::string& gameDataName, const Json::ComponentSerializationHolder& jsonSerializerHolder)
	{
		SCOPED_PROFILER("LoadGameData");

		namespace fs = std::filesystem;
		fs::path gameDataPath(gameDataName);

		// if it's name, we search the map in maps folder
		if (gameDataName.find_first_of("/\\.") == std::string::npos)
		{
			gameDataPath = workingPath / GAME_DATA_PATH / (gameDataName + ".json");
		}

		try
		{
			std::ifstream mapFile(gameDataPath);
			nlohmann::json mapJson;
			mapFile >> mapJson;

			if (const auto& worldObject = mapJson.at("gameData"); worldObject.is_object())
			{
				gameData.fromJson(worldObject, jsonSerializerHolder);
			}
		}
		catch (const nlohmann::detail::exception& e)
		{
			ReportError("Can't parse gameData '%s': %s", gameDataPath.c_str(), e.what());
		}
		catch (const std::exception& e)
		{
			ReportError("Can't open gameData '%s': %s", gameDataPath.c_str(), e.what());
		}
	}
} // namespace GameDataLoader
