#pragma once

#include <raccoon-ecs/system.h>

class WorldHolder;

/**
 * System that sends commands generated during the frame
 */
class ServerCommandsSendSystem : public RaccoonEcs::System
{
public:
	ServerCommandsSendSystem(WorldHolder& worldHolder) noexcept;

	void update() override;

private:
	WorldHolder& mWorldHolder;
};
