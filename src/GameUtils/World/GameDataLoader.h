#pragma once

#include <filesystem>
#include <string>

#include "GameData/EcsDefinitions.h"

class WorldLayer;
class GameData;

namespace Json
{
	class ComponentSerializationHolder;
}

namespace GameDataLoader
{
	void SaveWorld(WorldLayer& world, const std::filesystem::path& appFolder, const std::string& levelName, const Json::ComponentSerializationHolder& jsonSerializerHolder);
	void LoadWorld(WorldLayer& world, const std::filesystem::path& appFolder, const std::string& levelName, const Json::ComponentSerializationHolder& jsonSerializerHolder);

	void SaveGameData(const GameData& gameData, const std::filesystem::path& appFolder, const std::string& gameDataName, const Json::ComponentSerializationHolder& jsonSerializerHolder);
	void LoadGameData(GameData& gameData, const std::filesystem::path& appFolder, const std::string& gameDataName, const Json::ComponentSerializationHolder& jsonSerializerHolder);
}
