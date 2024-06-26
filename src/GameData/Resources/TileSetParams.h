#pragma once

#include <vector>

#include "GameData/Resources/ResourceHandle.h"

struct TileSetParams
{
	static constexpr size_t EmptyTileIndex = std::numeric_limits<size_t>::max();

	enum class Material
	{
		None,
		Brick,
		Metal,
		Water,
		Foliage
	};

	struct Tile
	{
		ResourceHandle spriteHandle;
		Material material = Material::None;
	};

	std::vector<Tile> tiles;
	std::unordered_map<size_t, size_t> indexesConversion;
};
