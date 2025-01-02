#pragma once

#include <raccoon-ecs/utils/system.h>

class WorldHolder;

/**
 * System that saves movement data for future interpolation
 */
class SaveMovementForInterpolationSystem final : public RaccoonEcs::System
{
public:
	explicit SaveMovementForInterpolationSystem(WorldHolder& worldHolder) noexcept;

	void update() override;

private:
	WorldHolder& mWorldHolder;
};
