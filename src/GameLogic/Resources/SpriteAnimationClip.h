#pragma once

#ifndef DISABLE_SDL

#include <memory>
#include <vector>

#include "GameData/Resources/Resource.h"
#include "GameData/Resources/ResourceHandle.h"

namespace Graphics
{
	class SpriteAnimationClip : public Resource
	{
	public:
		SpriteAnimationClip() = default;
		explicit SpriteAnimationClip(std::vector<ResourceHandle>&& sprites);

		bool isValid() const override;

		const ResourceHandle& getSprite(float progress) const;
		const std::vector<ResourceHandle>& getSprites() const;

		static std::string GetUniqueId(const RelativeResourcePath& filename);
		static InitSteps GetInitSteps();
		DeinitSteps getDeinitSteps() const override;

	private:
		std::vector<ResourceHandle> mSprites;
	};
} // namespace Graphics

#endif // !DISABLE_SDL
