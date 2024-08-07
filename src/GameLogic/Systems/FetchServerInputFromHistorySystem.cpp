#include "EngineCommon/precomp.h"

#include "GameLogic/Systems/FetchServerInputFromHistorySystem.h"

#include "GameData/Components/GameplayInputComponent.generated.h"
#include "GameData/Components/ServerConnectionsComponent.generated.h"
#include "GameData/Components/TimeComponent.generated.h"
#include "GameData/WorldLayer.h"

#include "GameUtils/Network/GameStateRewinder.h"
#include "GameUtils/SharedManagers/WorldHolder.h"

FetchServerInputFromHistorySystem::FetchServerInputFromHistorySystem(WorldHolder& worldHolder, GameStateRewinder& gameStateRewinder) noexcept
	: mWorldHolder(worldHolder)
	, mGameStateRewinder(gameStateRewinder)
{
}

void FetchServerInputFromHistorySystem::update()
{
	SCOPED_PROFILER("FetchServerInputFromHistorySystem::update");
	WorldLayer& world = mWorldHolder.getDynamicWorldLayer();

	const auto [time] = world.getWorldComponents().getComponents<const TimeComponent>();
	const u32 currentUpdateIndex = time->getValue()->lastFixedUpdateIndex;

	EntityManager& entityManager = world.getEntityManager();
	ServerConnectionsComponent* serverConnections = mGameStateRewinder.getNotRewindableComponents().getOrAddComponent<ServerConnectionsComponent>();
	for (auto [connectionId, oneClientData] : serverConnections->getClientData())
	{
		if (oneClientData.playerEntity.isValid())
		{
			const Entity playerEntity = oneClientData.playerEntity.getEntity();
			if (!entityManager.hasEntity(playerEntity))
			{
				ReportError("Player has entity assigned but it doesn't exist, connectionId=%u, entity id=%u", connectionId, playerEntity.getRawId());
				continue;
			}

			auto [gameplayInput] = entityManager.getEntityComponents<GameplayInputComponent>(playerEntity);
			if (gameplayInput == nullptr)
			{
				gameplayInput = entityManager.addComponent<GameplayInputComponent>(playerEntity);
			}
			const GameplayInput::FrameState& frameInput = mGameStateRewinder.getOrPredictPlayerInput(connectionId, currentUpdateIndex);
			gameplayInput->setCurrentFrameState(frameInput);
		}
	}
}
