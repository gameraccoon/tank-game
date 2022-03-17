#pragma once

#include "GameData/Resources/Resource.h"
#include "GameData/Resources/TileGridParams.h"

namespace Graphics
{
	class TileGrid : public Resource
	{
	public:
		TileGrid() = default;
		explicit TileGrid(const TileGridParams& params);

		bool isValid() const override;
		const TileGridParams& getTileGridParams() const;

		static std::string GetUniqueId(const std::string& filename);
		static InitSteps GetInitSteps();
		DeinitSteps getDeinitSteps() const override;

	private:
		TileGridParams mParams;
	};
}
