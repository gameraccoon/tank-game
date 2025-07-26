#include "EngineCommon/precomp.h"

#include "GameLogic/Systems/ServerNetworkConnectionSystem.h"

#include "GameData/Components/ServerNetworkInterfaceComponent.generated.h"
#include "GameData/GameData.h"
#include "GameData/WorldLayer.h"

#include "GameUtils/SharedManagers/WorldHolder.h"

ServerNetworkConnectionSystem::ServerNetworkConnectionSystem(
	WorldHolder& worldHolder,
	const u16 serverPort
) noexcept
	: mWorldHolder(worldHolder)
	, mServerPort(serverPort)
{
}

void ServerNetworkConnectionSystem::update()
{
	SCOPED_PROFILER("ServerNetworkConnectionSystem::update");

	GameData& gameData = mWorldHolder.getGameData();

	auto [networkInterface] = gameData.getGameComponents().getComponents<ServerNetworkInterfaceComponent>();

	if (!networkInterface->getNetwork().isValid())
	{
		return;
	}

	if (!networkInterface->getNetwork().isPortOpen(mServerPort))
	{
		networkInterface->getNetworkRef().startListeningToPort(mServerPort);
	}
}
