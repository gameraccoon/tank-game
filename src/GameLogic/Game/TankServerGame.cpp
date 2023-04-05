#include "Base/precomp.h"

#include "GameLogic/Game/TankServerGame.h"

#include "Base/Types/TemplateHelpers.h"

#include "GameData/ComponentRegistration/ComponentFactoryRegistration.h"
#include "GameData/ComponentRegistration/ComponentJsonSerializerRegistration.h"

#include "GameData/Components/ConnectionManagerComponent.generated.h"
#include "GameData/Components/GameplayInputComponent.generated.h"
#include "GameData/Components/RenderAccessorComponent.generated.h"
#include "GameData/Components/ServerConnectionsComponent.generated.h"
#include "GameData/Components/TimeComponent.generated.h"
#include "GameData/Components/WorldCachedDataComponent.generated.h"

#include "Utils/Application/ArgumentsParser.h"
#include "Utils/ResourceManagement/ResourceManager.h"
#include "Utils/World/GameDataLoader.h"

#include "HAL/Base/Engine.h"

#include "GameLogic/Initialization/StateMachines.h"
#include "GameLogic/Render/RenderAccessor.h"
#include "GameLogic/Systems/AnimationSystem.h"
#include "GameLogic/Systems/ApplyGameplayCommandsSystem.h"
#include "GameLogic/Systems/CharacterStateSystem.h"
#include "GameLogic/Systems/ControlSystem.h"
#include "GameLogic/Systems/DeadEntitiesDestructionSystem.h"
#include "GameLogic/Systems/FetchScheduledCommandsSystem.h"
#include "GameLogic/Systems/MovementSystem.h"
#include "GameLogic/Systems/RenderSystem.h"
#include "GameLogic/Systems/ResourceStreamingSystem.h"
#include "GameLogic/Systems/SaveCommandsToHistorySystem.h"
#include "GameLogic/Systems/ServerCommandsSendSystem.h"
#include "GameLogic/Systems/ServerMovesSendSystem.h"
#include "GameLogic/Systems/ServerNetworkSystem.h"

TankServerGame::TankServerGame(ResourceManager& resourceManager, ThreadPool& threadPool)
	: Game(nullptr, resourceManager, threadPool)
{
}

void TankServerGame::preStart(const ArgumentsParser& arguments, std::optional<RenderAccessorGameRef> renderAccessor)
{
	SCOPED_PROFILER("TankServerGame::preStart");

	ComponentsRegistration::RegisterComponents(getComponentFactory());
	ComponentsRegistration::RegisterJsonSerializers(getComponentSerializers());

	mServerPort = static_cast<u16>(arguments.getIntArgumentValue("open-port", 14436));

	getWorldHolder().setWorld(mGameStateRewinder.getWorld(mGameStateRewinder.getTimeData().lastFixedUpdateIndex));

	const bool shouldRender = renderAccessor.has_value();

	initSystems(shouldRender);

	GameDataLoader::LoadWorld(getWorldHolder().getWorld(), arguments.getArgumentValue("world", "test"), getComponentSerializers());
	GameDataLoader::LoadGameData(getGameData(), arguments.getArgumentValue("gameData", "gameData"), getComponentSerializers());

	TimeComponent* timeComponent = getWorldHolder().getWorld().getWorldComponents().addComponent<TimeComponent>();
	timeComponent->setValue(&mGameStateRewinder.getTimeData());

	// if we do debug rendering of server state
	if (shouldRender)
	{
		RenderAccessorComponent* renderAccessorComponent = getGameData().getGameComponents().addComponent<RenderAccessorComponent>();
		renderAccessorComponent->setAccessor(renderAccessor);
		getWorldHolder().getWorld().getWorldComponents().getOrAddComponent<WorldCachedDataComponent>()->setDrawShift(Vector2D(300.0f, 0.0f));
	}
	{
		ConnectionManagerComponent* connectionManager = getWorldHolder().getGameData().getGameComponents().addComponent<ConnectionManagerComponent>();
		connectionManager->setManagerPtr(&mConnectionManager);
	}

	Game::preStart(arguments);
}

void TankServerGame::initResources()
{
	SCOPED_PROFILER("TankServerGame::initResources");
	getResourceManager().loadAtlasesData("resources/atlas/atlas-list.json");
	Game::initResources();
}

void TankServerGame::dynamicTimePreFrameUpdate(float dt, int plannedFixedTimeUpdates)
{
	SCOPED_PROFILER("TankServerGame::dynamicTimePreFrameUpdate");
	Game::dynamicTimePreFrameUpdate(dt, plannedFixedTimeUpdates);

	mConnectionManager.processNetworkEvents();

	processInputCorrections();
}

void TankServerGame::fixedTimeUpdate(float dt)
{
	SCOPED_PROFILER("TankServerGame::fixedTimeUpdate");

	const auto [time] = getWorldHolder().getWorld().getWorldComponents().getComponents<const TimeComponent>();
	applyInputForCurrentUpdate(time->getValue()->lastFixedUpdateIndex + 1);
	mGameStateRewinder.advanceSimulationToNextUpdate(time->getValue()->lastFixedUpdateIndex + 1);
	getWorldHolder().setWorld(mGameStateRewinder.getWorld(mGameStateRewinder.getTimeData().lastFixedUpdateIndex));

	Game::fixedTimeUpdate(dt);
}

void TankServerGame::dynamicTimePostFrameUpdate(float dt, int processedFixedTimeUpdates)
{
	SCOPED_PROFILER("TankServerGame::dynamicTimePostFrameUpdate");

	Game::dynamicTimePostFrameUpdate(dt, processedFixedTimeUpdates);

	mConnectionManager.flushMesssagesForAllClientConnections(14436);
}

TimeData& TankServerGame::getTimeData()
{
	return mGameStateRewinder.getTimeData();
}

void TankServerGame::initSystems([[maybe_unused]] bool shouldRender)
{
	SCOPED_PROFILER("TankServerGame::initSystems");

	getPreFrameSystemsManager().registerSystem<ServerNetworkSystem>(getWorldHolder(), mGameStateRewinder, mServerPort, mShouldQuitGame);
	getGameLogicSystemsManager().registerSystem<ApplyGameplayCommandsSystem>(getWorldHolder(), mGameStateRewinder);
	getGameLogicSystemsManager().registerSystem<FetchScheduledCommandsSystem>(getWorldHolder(), mGameStateRewinder);
	getGameLogicSystemsManager().registerSystem<ControlSystem>(getWorldHolder());
	getGameLogicSystemsManager().registerSystem<DeadEntitiesDestructionSystem>(getWorldHolder());
	getGameLogicSystemsManager().registerSystem<MovementSystem>(getWorldHolder());
	getGameLogicSystemsManager().registerSystem<CharacterStateSystem>(getWorldHolder());
	getGameLogicSystemsManager().registerSystem<AnimationSystem>(getWorldHolder());
	getGameLogicSystemsManager().registerSystem<SaveCommandsToHistorySystem>(getWorldHolder(), mGameStateRewinder);
	getPostFrameSystemsManager().registerSystem<ServerMovesSendSystem>(getWorldHolder(), mGameStateRewinder);
	getPostFrameSystemsManager().registerSystem<ServerCommandsSendSystem>(getWorldHolder(), mGameStateRewinder);
	getPostFrameSystemsManager().registerSystem<ResourceStreamingSystem>(getWorldHolder(), getResourceManager());

#ifndef DEDICATED_SERVER
	if (shouldRender)
	{
		getPostFrameSystemsManager().registerSystem<RenderSystem>(getWorldHolder(), getResourceManager(), getThreadPool());
	}
#endif // !DEDICATED_SERVER
}

void TankServerGame::correctUpdates(u32 firstIncorrectUpdateIdx)
{
	SCOPED_PROFILER("TankServerGame::correctUpdates");

	const auto [time] = getWorldHolder().getWorld().getWorldComponents().getComponents<const TimeComponent>();
	const TimeData& timeValue = *time->getValue();
	const float fixedUpdateDt = timeValue.lastFixedUpdateDt;

	const u32 lastFixedUpdateIndex = timeValue.lastFixedUpdateIndex;

	AssertFatal(firstIncorrectUpdateIdx > 0, "We can't correct the baseline zero update");
	AssertFatal(firstIncorrectUpdateIdx <= lastFixedUpdateIndex, "We can't correct updates from the future");

	LogInfo("Correct server updates from %u to %u", firstIncorrectUpdateIdx, lastFixedUpdateIndex);

	mGameStateRewinder.unwindBackInHistory(firstIncorrectUpdateIdx);

	for (u32 i = firstIncorrectUpdateIdx; i <= lastFixedUpdateIndex; ++i)
	{
		fixedTimeUpdate(fixedUpdateDt);
	}
}

void TankServerGame::processInputCorrections()
{
	SCOPED_PROFILER("TankServerGame::processInputCorrections");

	const auto [time] = getWorldHolder().getWorld().getWorldComponents().getComponents<const TimeComponent>();

	// first real update has index 1
	if (time->getValue()->lastFixedUpdateIndex < 1)
	{
		return;
	}

	const u32 lastProcessedUpdateIdx = time->getValue()->lastFixedUpdateIndex;

	const u32 firstUpdateToCorrect = mGameStateRewinder.getFirstDesyncedUpdateIdx();

	if (firstUpdateToCorrect <= lastProcessedUpdateIdx)
	{
		correctUpdates(firstUpdateToCorrect);
	}

	ServerConnectionsComponent* serverConnections = mGameStateRewinder.getNotRewindableComponents().getOrAddComponent<ServerConnectionsComponent>();
	std::vector<ConnectionId> players;
	players.reserve(serverConnections->getClientData().size());
	for (auto [connectionId, oneClientData] : serverConnections->getClientData()) {
		players.push_back(connectionId);
	}

	const u32 firstUpdateToKeep = mGameStateRewinder.getLastKnownInputUpdateIdxForPlayers(players);
	mGameStateRewinder.trimOldFrames(std::min(firstUpdateToKeep, lastProcessedUpdateIdx));
}

void TankServerGame::applyInputForCurrentUpdate(u32 inputUpdateIndex)
{
	SCOPED_PROFILER("TankServerGame::applyInputForCurrentUpdate");
	EntityManager& entityManager = getWorldHolder().getWorld().getEntityManager();
	ServerConnectionsComponent* serverConnections = mGameStateRewinder.getNotRewindableComponents().getOrAddComponent<ServerConnectionsComponent>();
	for (auto [connectionId, oneClientData] : serverConnections->getClientData())
	{
		if (oneClientData.playerEntity.isValid())
		{
			const Entity playerEntity = oneClientData.playerEntity.getEntity();
			auto [gameplayInput] = entityManager.getEntityComponents<GameplayInputComponent>(playerEntity);
			if (gameplayInput == nullptr)
			{
				gameplayInput = entityManager.addComponent<GameplayInputComponent>(playerEntity);
			}
			const GameplayInput::FrameState& frameInput = mGameStateRewinder.getOrPredictPlayerInput(connectionId, inputUpdateIndex);
			gameplayInput->setCurrentFrameState(frameInput);
		}
	}
}
