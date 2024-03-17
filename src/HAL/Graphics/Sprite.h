#pragma once

#ifndef DISABLE_SDL

#include "HAL/EngineFwd.h"

#include "GameData/Resources/Resource.h"

#include "HAL/Base/Types.h"

class RelativeResourcePath;

namespace HAL::Graphics
{
	class Surface;

	class Sprite : public Resource
	{
	public:
		Sprite(const Surface* surface, QuadUV uv);

		int getHeight() const;
		int getWidth() const;

		const Surface* getSurface() const;
		QuadUV getUV() const { return mUV; }

		bool isValid() const override;

		static std::string GetUniqueId(const RelativeResourcePath& filename);
		static InitSteps GetInitSteps();
		DeinitSteps getDeinitSteps() const override;

	private:
		const Surface* mSurface = nullptr;
		QuadUV mUV;
	};
} // namespace HAL::Graphics

#endif // !DISABLE_SDL
