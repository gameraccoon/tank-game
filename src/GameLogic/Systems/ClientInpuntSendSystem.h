#pragma once

#include <raccoon-ecs/system.h>

class WorldHolder;

/**
 * System that updates and sends input history to server
 */
class ClientInputSendSystem : public RaccoonEcs::System
{
public:
	ClientInputSendSystem(WorldHolder& worldHolder) noexcept;

	void update() override;

private:
	WorldHolder& mWorldHolder;
};
