#pragma once

#include <vector>
#include <limits>

#include "GameData/Geometry/IntVector2D.h"
#include "GameData/Resources/ResourceHandle.h"
#include "GameData/Resources/TileSetParams.h"

struct TileGridParams
{
	std::vector<TileSetParams::Tile> tiles;
	IntVector2D originalTileSize = {16, 16};
	IntVector2D gridSize {1, 1};
	// vector of grids from back to front, the grid cells are adressed as (y*width + x)
	// the values are indexes in the corresponding tile set
	std::vector<std::vector<size_t>> layers;
};
