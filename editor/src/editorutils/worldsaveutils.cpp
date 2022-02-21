#include "worldsaveutils.h"

#include <algorithm>
#include <ranges>

#include "GameData/World.h"

#include "Utils/World/GameDataLoader.h"

namespace Utils
{
	void SaveWorld(World& world, const std::string& fileName, const Json::ComponentSerializationHolder& jsonSerializerHolder)
	{
		world.clearCaches();
		GameDataLoader::SaveWorld(world, fileName, jsonSerializerHolder);
	}
}
