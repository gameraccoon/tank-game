#include "Base/precomp.h"

#include "GameLogic/Resources/TileSet.h"

#include <filesystem>

#include <nlohmann/json.hpp>

#include "Base/Types/String/Path.h"

#include "GameData/Resources/TileSetParams.h"

#include "Utils/ResourceManagement/ResourceManager.h"

#include "HAL/Graphics/Sprite.h"

#include "GameLogic/Resources/SpriteAnimationClip.h"

namespace Graphics
{
	static TileSetParams::Material GetTileSetMaterialFromProperties(const nlohmann::json& propertiesJson)
	{
		static std::unordered_map<std::string, TileSetParams::Material> stringToEnumMapping {
			{ "brick", TileSetParams::Material::Brick },
			{ "metal", TileSetParams::Material::Metal },
			{ "water", TileSetParams::Material::Water },
			{ "foliage", TileSetParams::Material::Foliage },
		};

		if (propertiesJson.empty())
		{
			return TileSetParams::Material::None;
		}

		return stringToEnumMapping[propertiesJson.at(0).at("value").get<std::string>()];
	}

	static TileSetParams LoadTileSetJson(const ResourcePath& path, ResourceManager& resourceManager)
	{
		SCOPED_PROFILER("LoadTileSetJson");
		namespace fs = std::filesystem;
		fs::path atlasDescPath(static_cast<std::string>(path));

		TileSetParams result;
		ResourcePath pathBase;

		try
		{
			std::ifstream tileSetFile(atlasDescPath);
			nlohmann::json tileSetJson;
			tileSetFile >> tileSetJson;

			result.indexesConversion[0] = TileSetParams::EmptyTileIndex;

			for (auto& tileJson : tileSetJson.at("tiles"))
			{
				const int id = tileJson.at("id").get<int>();
				result.indexesConversion[id] = result.tiles.size();
				std::string imagePath = tileJson.at("image").get<std::string>();
				ResourceHandle spriteHandle = resourceManager.lockResource<Sprite>(ResourcePath{ std::string("resources") + imagePath.substr(2)});
				result.tiles.emplace_back(spriteHandle, GetTileSetMaterialFromProperties(tileJson.at("properties")));
			}
		}
		catch(const std::exception& e)
		{
			LogError("Can't open tile set file '%s': %s", path.c_str(), e.what());
		}

		return result;
	}

	static UniqueAny CreateTileSet(UniqueAny&& resource, ResourceManager& resourceManager, ResourceHandle)
	{
		SCOPED_PROFILER("CalculateSpriteDependencies");

		const ResourcePath* pathPtr = resource.cast<ResourcePath>();

		if (!pathPtr)
		{
			ReportFatalError("We got an incorrect type of value when loading a sprite in CalculateSpriteDependencies (expected ResourcePath)");
			return {};
		}

		const ResourcePath& path = *pathPtr;

		return UniqueAny::Create<Resource::Ptr>(std::make_unique<TileSet>(LoadTileSetJson(path, resourceManager)));
	}

	TileSet::TileSet(TileSetParams&& params)
		: mParams(std::move(params))
	{
	}

	const TileSetParams& TileSet::getTileSetParams() const
	{
		return mParams;
	}

	bool TileSet::isValid() const
	{
		return true;
	}

	std::string TileSet::GetUniqueId(const std::string& filename)
	{
		return filename;
	}

	Resource::InitSteps TileSet::GetInitSteps()
	{
		return {
			InitStep{
				.thread = Thread::Any,
				.init = &CreateTileSet,
			},
		};
		return {};
	}

	Resource::DeinitSteps TileSet::getDeinitSteps() const
	{
		return {};
	}
}
