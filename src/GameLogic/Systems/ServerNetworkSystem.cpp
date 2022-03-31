#include "Base/precomp.h"

#include "GameLogic/Systems/ServerNetworkSystem.h"

#include <sdl/SDL_keycode.h>
#include <sdl/SDL_mouse.h>

#include "GameData/World.h"
#include "GameData/GameData.h"
#include "GameData/Components/TransformComponent.generated.h"
#include "GameData/Components/MovementComponent.generated.h"
#include "GameData/Components/RenderModeComponent.generated.h"
#include "GameData/Components/ImguiComponent.generated.h"
#include "GameData/Components/CharacterStateComponent.generated.h"


ServerNetworkSystem::ServerNetworkSystem(WorldHolder& worldHolder, HAL::ConnectionManager& connectionManager, bool& shouldQuitGame) noexcept
	: mWorldHolder(worldHolder)
	, mConnectionManager(connectionManager)
	, mShouldQuitGame(shouldQuitGame)
{
}

void ServerNetworkSystem::update()
{
	SCOPED_PROFILER("ServerNetworkSystem::update");

	if (!mConnectionManager.isPortOpen(12345))
	{
		mConnectionManager.startListeningToPort(12345);
		return;
	}

	// for now quit on any message
	auto newMessages = mConnectionManager.consumeReceivedMessages(12345);
	if (!newMessages.empty())
	{
		mShouldQuitGame = true;
	}
}
