#pragma once

#include <raccoon-ecs/system.h>

class WorldHolder;

/**
 * System that processes gameplay controls
 */
class ControlSystem : public RaccoonEcs::System
{
public:
	ControlSystem(WorldHolder& worldHolder) noexcept;

	void update() override;
	static std::string GetSystemId() { return "ControlSystem"; }

private:
	WorldHolder& mWorldHolder;
};
