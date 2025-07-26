#pragma once

#include <raccoon-ecs/utils/system.h>

#include "EngineCommon/Types/BasicTypes.h"

class WorldHolder;

/**
 * System that makes sure the server is ready to accept connections
 */
class ServerNetworkConnectionSystem final : public RaccoonEcs::System
{
public:
	ServerNetworkConnectionSystem(
		WorldHolder& worldHolder,
		u16 serverPort
	) noexcept;

	void update() override;

private:
	WorldHolder& mWorldHolder;
	const u16 mServerPort;
};
