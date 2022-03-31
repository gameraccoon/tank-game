#include "Base/precomp.h"

#include "GameLogic/Systems/ClientNetworkSystem.h"

#include <sdl/SDL_keycode.h>
#include <sdl/SDL_mouse.h>

#include "GameData/World.h"
#include "GameData/GameData.h"
#include "GameData/Components/TransformComponent.generated.h"
#include "GameData/Components/MovementComponent.generated.h"
#include "GameData/Components/RenderModeComponent.generated.h"
#include "GameData/Components/ImguiComponent.generated.h"
#include "GameData/Components/CharacterStateComponent.generated.h"


ClientNetworkSystem::ClientNetworkSystem(WorldHolder& worldHolder, HAL::ConnectionManager& connectionManager, const bool& shouldQuitGame) noexcept
	: mWorldHolder(worldHolder)
	, mConnectionManager(connectionManager)
	, mShouldQuitGameRef(shouldQuitGame)
{
}

void ClientNetworkSystem::update()
{
	SCOPED_PROFILER("ClientNetworkSystem::update");

	if (mConnectionId == HAL::ConnectionManager::InvalidConnectionId || !mConnectionManager.isConnectionOpen(mConnectionId))
	{
		const auto result = mConnectionManager.connectToServer(HAL::ConnectionManager::LocalhostV4, 12345);
		if (result.status == HAL::ConnectionManager::ConnectResult::Status::Success)
		{
			mConnectionId = result.connectionId;
		}
	}

	if (mShouldQuitGameRef && mConnectionId != HAL::ConnectionManager::InvalidConnectionId && mConnectionManager.isConnectionOpen(mConnectionId))
	{
		mConnectionManager.sendMessage(mConnectionId, HAL::ConnectionManager::Message{0, {}});
	}
}
