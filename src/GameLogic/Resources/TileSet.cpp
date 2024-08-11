#include "EngineCommon/precomp.h"

#include "GameLogic/Resources/TileSet.h"

#include <filesystem>

#include <nlohmann/json.hpp>

#include "EngineCommon/Types/String/ResourcePath.h"

#include "GameData/Resources/TileSetParams.h"

#include "HAL/Graphics/Sprite.h"

#include "GameUtils/ResourceManagement/ResourceManager.h"

namespace Graphics
{
	static TileSetParams::Material GetTileSetMaterialFromProperties(const nlohmann::json& propertiesJson)
	{
		static std::unordered_map<std::string, TileSetParams::Material> stringToEnumMapping{
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

	static TileSetParams LoadTileSetJson(const AbsoluteResourcePath& path, [[maybe_unused]] ResourceManager& resourceManager)
	{
		SCOPED_PROFILER("LoadTileSetJson");
		TileSetParams result;
		AbsoluteResourcePath pathBase;

		try
		{
			std::ifstream tileSetFile(path.getAbsolutePath());
			nlohmann::json tileSetJson;
			tileSetFile >> tileSetJson;

			result.indexesConversion[0] = TileSetParams::EmptyTileIndex;

			for (auto& tileJson : tileSetJson.at("tiles"))
			{
				const int id = tileJson.at("id").get<int>();
				result.indexesConversion[id] = result.tiles.size();
#ifndef DISABLE_SDL
				std::string imagePath = tileJson.at("image").get<std::string>();
				ResourceHandle spriteHandle = resourceManager.lockResource<Sprite>(RelativeResourcePath{ std::string("resources") + imagePath.substr(2) });
				result.tiles.emplace_back(spriteHandle, GetTileSetMaterialFromProperties(tileJson.at("properties")));
#else
				result.tiles.emplace_back(ResourceHandle(), GetTileSetMaterialFromProperties(tileJson.at("properties")));
#endif // !DISABLE_SDL
			}
		}
		catch (const std::exception& e)
		{
			LogError("Can't open tile set file '%s': %s", path.getAbsolutePath().c_str(), e.what());
		}

		return result;
	}

	static UniqueAny CreateTileSet(UniqueAny&& resource, ResourceManager& resourceManager, ResourceHandle)
	{
		SCOPED_PROFILER("CalculateSpriteDependencies");

		const RelativeResourcePath* pathPtr = resource.cast<RelativeResourcePath>();

		if (!pathPtr)
		{
			ReportFatalError("We got an incorrect type of value when loading a sprite in CalculateSpriteDependencies (expected ResourcePath)");
			return {};
		}

		const AbsoluteResourcePath& path = resourceManager.getAbsoluteResourcePath(*pathPtr);

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

	std::string TileSet::GetUniqueId(const RelativeResourcePath& filename)
	{
		return filename.getRelativePathStr();
	}

	Resource::InitSteps TileSet::GetInitSteps()
	{
		return {
			InitStep{
				.thread = Thread::Any,
				.init = &CreateTileSet,
			},
		};
	}

	Resource::DeinitSteps TileSet::getDeinitSteps() const
	{
		return {};
	}
} // namespace Graphics
