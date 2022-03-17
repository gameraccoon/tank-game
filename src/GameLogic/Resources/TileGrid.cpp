#include "Base/precomp.h"

#include "GameLogic/Resources/TileGrid.h"

#include <filesystem>

#include <nlohmann/json.hpp>

#include "Base/Types/String/Path.h"

#include "GameData/Resources/TileGridParams.h"

#include "Utils/ResourceManagement/ResourceManager.h"

#include "GameLogic/Resources/TileSet.h"

namespace Graphics
{
	struct TileGridLoadData
	{
		TileGridParams params;
		ResourceHandle tileSetHandle;
		size_t firstTileId;
	};

	static TileGridLoadData LoadTileGridFromJson(const ResourcePath& path, ResourceManager& resourceManager)
	{
		SCOPED_PROFILER("LoadTileGridFromJson");
		namespace fs = std::filesystem;
		fs::path tileGridPath(static_cast<std::string>(path));

		TileGridLoadData result;
		ResourcePath pathBase;

		try
		{
			std::ifstream tileGridFile(tileGridPath);
			nlohmann::json tileGridJson;
			tileGridFile >> tileGridJson;

			tileGridJson.at("width").get_to(result.params.gridSize.x);
			tileGridJson.at("height").get_to(result.params.gridSize.y);
			tileGridJson.at("tilewidth").get_to(result.params.originalTileSize.x);
			tileGridJson.at("tileheight").get_to(result.params.originalTileSize.y);

			for (auto& layer : tileGridJson.at("layers"))
			{
				result.params.layers.emplace_back();
				layer.at("data").get_to(result.params.layers.back());
			}

			Assert(tileGridJson.at("tilesets").size() == 1, "We can't use more or less than one tile set per tile grid");
			for (auto& tileset : tileGridJson.at("tilesets"))
			{
				const std::string path = tileset.at("source").get<std::string>();
				result.tileSetHandle = resourceManager.lockResource<TileSet>(ResourcePath(std::string("resources/tilesets/") + path));
				result.firstTileId = tileset.at("firstgid").get<size_t>();
			}
		}
		catch(const std::exception& e)
		{
			LogError("Can't open tile grid data '%s': %s", path.c_str(), e.what());
		}

		return result;
	}

	static UniqueAny LoadTileGridJsonAndRequestDependencies(UniqueAny&& resource, ResourceManager& resourceManager, ResourceHandle)
	{
		SCOPED_PROFILER("LoadTileGridJsonAndRequestDependencies");

		const ResourcePath* pathPtr = resource.cast<ResourcePath>();

		if (!pathPtr)
		{
			ReportFatalError("We got an incorrect type of value when loading a sprite in CalculateSpriteDependencies (expected ResourcePath)");
			return {};
		}

		const ResourcePath& path = *pathPtr;

		return UniqueAny::Create<TileGridLoadData>(LoadTileGridFromJson(path, resourceManager));
	}

	static UniqueAny CreateTileGrid(UniqueAny&& resource, ResourceManager& resourceManager, ResourceHandle)
	{
		SCOPED_PROFILER("CreateTileGrid");

		TileGridLoadData* tileGridLoadData = resource.cast<TileGridLoadData>();

		if (!tileGridLoadData)
		{
			ReportFatalError("We got an incorrect type of value when loading a sprite in CreateAnimationGroup");
			return {};
		}

		const TileSet* tileSet = resourceManager.tryGetResource<TileSet>(tileGridLoadData->tileSetHandle);
		if (!tileSet)
		{
			ReportError("TileSet should have been loaded before tile grid creation");
			return {};
		}

		tileGridLoadData->params.tiles = tileSet->getTileSetParams().tiles;

		for (std::vector<size_t>& layer : tileGridLoadData->params.layers)
		{
			for (size_t& tileIdxRef : layer)
			{
				auto it = tileSet->getTileSetParams().indexesConversion.find(tileIdxRef);
				if (it != tileSet->getTileSetParams().indexesConversion.end()) {
					tileIdxRef = it->second - tileGridLoadData->firstTileId;
				}
			}
		}

		return UniqueAny::Create<Resource::Ptr>(std::make_unique<TileGrid>(std::move(tileGridLoadData->params)));
	}

	TileGrid::TileGrid(const TileGridParams& params)
		: mParams(params)
	{
	}

	const TileGridParams& TileGrid::getTileGridParams() const
	{
		return mParams;
	}

	bool TileGrid::isValid() const
	{
		return true;
	}

	std::string TileGrid::GetUniqueId(const std::string& filename)
	{
		return filename;
	}

	Resource::InitSteps TileGrid::GetInitSteps()
	{
		return {
			InitStep{
				.thread = Thread::Any,
				.init = &LoadTileGridJsonAndRequestDependencies,
			},
			InitStep{
				.thread = Thread::Any,
				.init = &CreateTileGrid,
			},
		};
	}

	Resource::DeinitSteps TileGrid::getDeinitSteps() const
	{
		return {};
	}
}
