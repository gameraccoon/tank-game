#include "EngineCommon/precomp.h"

#include "GameLogic/Game/TankServerGame.h"

#include "EngineCommon/Types/TemplateHelpers.h"

#include "GameData/ComponentRegistration/ComponentFactoryRegistration.h"
#include "GameData/ComponentRegistration/ComponentJsonSerializerRegistration.h"
#include "GameData/Components/ConnectionManagerComponent.generated.h"
#include "GameData/Components/RenderAccessorComponent.generated.h"
#include "GameData/Components/ServerConnectionsComponent.generated.h"
#include "GameData/Components/TimeComponent.generated.h"

#include "HAL/Base/Engine.h"

#include "GameUtils/Application/ArgumentsParser.h"
#include "GameUtils/ResourceManagement/ResourceManager.h"
#include "GameUtils/World/GameDataLoader.h"

#include "GameLogic/Render/RenderAccessor.h"
#include "GameLogic/Systems/AnimationSystem.h"
#include "GameLogic/Systems/ApplyGameplayCommandsSystem.h"
#include "GameLogic/Systems/CharacterStateSystem.h"
#include "GameLogic/Systems/CollisionResolutionSystem.h"
#include "GameLogic/Systems/ControlSystem.h"
#include "GameLogic/Systems/DeadEntitiesDestructionSystem.h"
#include "GameLogic/Systems/FetchExternalCommandsSystem.h"
#include "GameLogic/Systems/FetchServerInputFromHistorySystem.h"
#include "GameLogic/Systems/MovementSystem.h"
#include "GameLogic/Systems/ProjectileLifetimeSystem.h"
#include "GameLogic/Systems/RenderSystem.h"
#include "GameLogic/Systems/ResourceStreamingSystem.h"
#include "GameLogic/Systems/SaveCommandsToHistorySystem.h"
#include "GameLogic/Systems/ServerCommandsSendSystem.h"
#include "GameLogic/Systems/ServerMovesSendSystem.h"
#include "GameLogic/Systems/ServerNetworkSystem.h"
#include "GameLogic/Systems/ShootingSystem.h"

TankServerGame::TankServerGame(ResourceManager& resourceManager, ThreadPool& threadPool, int instanceIndex) noexcept
	: Game(nullptr, resourceManager, threadPool, instanceIndex)
{
}

void TankServerGame::preStart(const ArgumentsParser& arguments, std::optional<RenderAccessorGameRef> renderAccessor)
{
	SCOPED_PROFILER("TankServerGame::preStart");

	ComponentsRegistration::RegisterComponents(getComponentFactory());
	ComponentsRegistration::RegisterJsonSerializers(getComponentSerializers());

	mServerPort = static_cast<u16>(arguments.getIntArgumentValue("open-port").getValueOr(14436));

	getWorldHolder().setDynamicWorld(mGameStateRewinder.getDynamicWorld(mGameStateRewinder.getTimeData().lastFixedUpdateIndex));

	const bool shouldRender = renderAccessor.has_value();

	initSystems(shouldRender);

	GameDataLoader::LoadWorld(getWorldHolder().getStaticWorldLayer(), std::filesystem::current_path(), arguments.getArgumentValue("world").value_or("test"), getComponentSerializers());
	GameDataLoader::LoadGameData(getGameData(), std::filesystem::current_path(), arguments.getArgumentValue("gameData").value_or("gameData"), getComponentSerializers());

	TimeComponent* timeComponent = getWorldHolder().getDynamicWorldLayer().getWorldComponents().addComponent<TimeComponent>();
	timeComponent->setValue(&mGameStateRewinder.getTimeData());

	// if we do debug rendering of server state
	if (shouldRender)
	{
		RenderAccessorComponent* renderAccessorComponent = getGameData().getGameComponents().addComponent<RenderAccessorComponent>();
		renderAccessorComponent->setAccessor(renderAccessor);
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
	getResourceManager().loadAtlasesData(RelativeResourcePath("resources/atlas/atlas-list.json"));
	Game::initResources();
}

void TankServerGame::notPausablePreFrameUpdate(float dt)
{
	SCOPED_PROFILER("TankServerGame::notPausablePreFrameUpdate");

	mConnectionManager.processNetworkEvents();

	Game::notPausablePreFrameUpdate(dt);
}

void TankServerGame::dynamicTimePreFrameUpdate(float dt, int plannedFixedTimeUpdates)
{
	SCOPED_PROFILER("TankServerGame::dynamicTimePreFrameUpdate");
	Game::dynamicTimePreFrameUpdate(dt, plannedFixedTimeUpdates);

	updateHistory();
}

void TankServerGame::fixedTimeUpdate(float dt)
{
	SCOPED_PROFILER("TankServerGame::fixedTimeUpdate");

	const auto [time] = getWorldHolder().getDynamicWorldLayer().getWorldComponents().getComponents<const TimeComponent>();
	const u32 thisUpdateIdx = time->getValue()->lastFixedUpdateIndex + 1;
	mGameStateRewinder.advanceSimulationToNextUpdate(thisUpdateIdx);
	getWorldHolder().setDynamicWorld(mGameStateRewinder.getDynamicWorld(thisUpdateIdx));

	Game::fixedTimeUpdate(dt);
}

void TankServerGame::notPausablePostFrameUpdate(float dt)
{
	SCOPED_PROFILER("TankServerGame::notPausablePostFrameUpdate");

	Game::notPausablePostFrameUpdate(dt);

	mConnectionManager.flushMessagesForAllClientConnections(mServerPort);
}

TimeData& TankServerGame::getTimeData()
{
	return mGameStateRewinder.getTimeData();
}

void TankServerGame::initSystems([[maybe_unused]] bool shouldRender)
{
	SCOPED_PROFILER("TankServerGame::initSystems");

	getNotPausablePreFrameSystemsManager().registerSystem<ServerNetworkSystem>(getWorldHolder(), mGameStateRewinder, mServerPort, mShouldPauseGame, mShouldQuitGame);

	getGameLogicSystemsManager().registerSystem<FetchServerInputFromHistorySystem>(getWorldHolder(), mGameStateRewinder);
	getGameLogicSystemsManager().registerSystem<FetchExternalCommandsSystem>(getWorldHolder(), mGameStateRewinder);
	getGameLogicSystemsManager().registerSystem<ApplyGameplayCommandsSystem>(getWorldHolder(), mGameStateRewinder);
	getGameLogicSystemsManager().registerSystem<ControlSystem>(getWorldHolder());
	getGameLogicSystemsManager().registerSystem<ProjectileLifetimeSystem>(getWorldHolder());
	getGameLogicSystemsManager().registerSystem<DeadEntitiesDestructionSystem>(getWorldHolder());
	getGameLogicSystemsManager().registerSystem<CharacterStateSystem>(getWorldHolder());
	getGameLogicSystemsManager().registerSystem<MovementSystem>(getWorldHolder());
	getGameLogicSystemsManager().registerSystem<CollisionResolutionSystem>(getWorldHolder());
	getGameLogicSystemsManager().registerSystem<ShootingSystem>(getWorldHolder());
	getGameLogicSystemsManager().registerSystem<AnimationSystem>(getWorldHolder());
	getGameLogicSystemsManager().registerSystem<SaveCommandsToHistorySystem>(getWorldHolder(), mGameStateRewinder);

	getPostFrameSystemsManager().registerSystem<ServerMovesSendSystem>(getWorldHolder(), mGameStateRewinder);
	getPostFrameSystemsManager().registerSystem<ServerCommandsSendSystem>(getWorldHolder(), mGameStateRewinder);
	getPostFrameSystemsManager().registerSystem<ResourceStreamingSystem>(getWorldHolder(), getResourceManager());

#ifndef DISABLE_SDL
	if (shouldRender)
	{
		getNotPausablePostFrameSystemsManager().registerSystem<RenderSystem>(getWorldHolder(), getResourceManager());
	}
#endif // !DISABLE_SDL
}

void TankServerGame::updateHistory()
{
	SCOPED_PROFILER("TankServerGame::updateHistory");

	const auto [time] = getWorldHolder().getDynamicWorldLayer().getWorldComponents().getComponents<const TimeComponent>();

	const u32 lastProcessedUpdateIdx = time->getValue()->lastFixedUpdateIndex;

	ServerConnectionsComponent* serverConnections = mGameStateRewinder.getNotRewindableComponents().getOrAddComponent<ServerConnectionsComponent>();
	std::vector<std::pair<ConnectionId, s32>> players;
	players.reserve(serverConnections->getClientData().size());
	for (auto [connectionId, oneClientData] : serverConnections->getClientData())
	{
		players.emplace_back(connectionId, oneClientData.indexShift);
	}

	const std::optional<u32> firstUpdateToKeepOption = mGameStateRewinder.getLastKnownInputUpdateIdxForPlayers(players);
	const u32 firstUpdateToKeep = firstUpdateToKeepOption.value_or(mGameStateRewinder.getFirstStoredUpdateIdx());
	mGameStateRewinder.trimOldUpdates(std::min(firstUpdateToKeep, lastProcessedUpdateIdx));
}
