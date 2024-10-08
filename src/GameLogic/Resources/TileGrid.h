#pragma once

#include "EngineData/Resources/Resource.h"

#include "GameData/Resources/TileGridParams.h"

class AbsoluteResourcePath;

namespace Graphics
{
	class TileGrid final : public Resource
	{
	public:
		TileGrid() = default;
		explicit TileGrid(const TileGridParams& params);

		bool isValid() const override;
		const TileGridParams& getTileGridParams() const;

		static std::string GetUniqueId(const AbsoluteResourcePath& filename);
		static InitSteps GetInitSteps();
		DeinitSteps getDeinitSteps() const override;

	private:
		TileGridParams mParams;
	};
} // namespace Graphics
