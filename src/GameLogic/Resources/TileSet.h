#pragma once

#include "GameData/Resources/Resource.h"
#include "GameData/Resources/TileSetParams.h"

namespace Graphics
{
	class TileSet : public Resource
	{
	public:
		TileSet() = default;
		explicit TileSet(TileSetParams&& params);

		bool isValid() const override;
		const TileSetParams& getTileSetParams() const;

		static std::string GetUniqueId(const std::string& filename);
		static InitSteps GetInitSteps();
		DeinitSteps getDeinitSteps() const override;

	private:
		TileSetParams mParams;
	};
}
