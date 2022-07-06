#pragma once

#include <raccoon-ecs/system.h>

class WorldHolder;

/**
 * System that sends entity moves to client
 */
class ServerMovesSendSystem : public RaccoonEcs::System
{
public:
	ServerMovesSendSystem(WorldHolder& worldHolder) noexcept;

	void update() override;

private:
	WorldHolder& mWorldHolder;
};
