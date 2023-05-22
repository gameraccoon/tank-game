#include "Base/precomp.h"

#include "GameLogic/Game/TankServerGame.h"

#include "Base/Types/TemplateHelpers.h"

#include "GameData/ComponentRegistration/ComponentFactoryRegistration.h"
#include "GameData/ComponentRegistration/ComponentJsonSerializerRegistration.h"

#include "GameData/Components/ConnectionManagerComponent.generated.h"
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
#include "GameLogic/Systems/FetchExternalCommandsSystem.h"
#include "GameLogic/Systems/FetchServerInputFromHistorySystem.h"
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

	updateHistory();
}

void TankServerGame::fixedTimeUpdate(float dt)
{
	SCOPED_PROFILER("TankServerGame::fixedTimeUpdate");

	const auto [time] = getWorldHolder().getWorld().getWorldComponents().getComponents<const TimeComponent>();
	const u32 thisUpdateIdx = time->getValue()->lastFixedUpdateIndex + 1;
	mGameStateRewinder.advanceSimulationToNextUpdate(thisUpdateIdx);
	getWorldHolder().setWorld(mGameStateRewinder.getWorld(thisUpdateIdx));

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
	getGameLogicSystemsManager().registerSystem<FetchServerInputFromHistorySystem>(getWorldHolder(), mGameStateRewinder);
	getGameLogicSystemsManager().registerSystem<FetchExternalCommandsSystem>(getWorldHolder(), mGameStateRewinder);
	getGameLogicSystemsManager().registerSystem<ApplyGameplayCommandsSystem>(getWorldHolder(), mGameStateRewinder);
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

void TankServerGame::updateHistory()
{
	SCOPED_PROFILER("TankServerGame::updateHistory");

	const auto [time] = getWorldHolder().getWorld().getWorldComponents().getComponents<const TimeComponent>();

	const u32 lastProcessedUpdateIdx = time->getValue()->lastFixedUpdateIndex;

	ServerConnectionsComponent* serverConnections = mGameStateRewinder.getNotRewindableComponents().getOrAddComponent<ServerConnectionsComponent>();
	std::vector<ConnectionId> players;
	players.reserve(serverConnections->getClientData().size());
	for (auto [connectionId, oneClientData] : serverConnections->getClientData())
	{
		players.push_back(connectionId);
	}

	const std::optional<u32> firstUpdateToKeepOption = mGameStateRewinder.getLastKnownInputUpdateIdxForPlayers(players);
	const u32 firstUpdateToKeep = firstUpdateToKeepOption.value_or(mGameStateRewinder.getFirstStoredUpdateIdx());
	mGameStateRewinder.trimOldFrames(std::min(firstUpdateToKeep, lastProcessedUpdateIdx));
}
