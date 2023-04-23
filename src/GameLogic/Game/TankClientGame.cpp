#include "Base/precomp.h"

#ifndef DEDICATED_SERVER

#include "GameLogic/Game/TankClientGame.h"

#include <unordered_map>

#include "Base/Types/TemplateHelpers.h"

#include "GameData/ComponentRegistration/ComponentFactoryRegistration.h"
#include "GameData/ComponentRegistration/ComponentJsonSerializerRegistration.h"

#include "GameData/Components/ClientGameDataComponent.generated.h"
#include "GameData/Components/ConnectionManagerComponent.generated.h"
#include "GameData/Components/GameplayCommandFactoryComponent.generated.h"
#include "GameData/Components/RenderAccessorComponent.generated.h"
#include "GameData/Components/TimeComponent.generated.h"

#include "Utils/Application/ArgumentsParser.h"
#include "Utils/Network/GameplayCommands/GameplayCommandFactoryRegistration.h"
#include "Utils/ResourceManagement/ResourceManager.h"
#include "Utils/World/GameDataLoader.h"

#include "HAL/Base/Engine.h"

#include "GameLogic/Systems/AnimationSystem.h"
#include "GameLogic/Systems/ApplyConfirmedMovesSystem.h"
#include "GameLogic/Systems/ApplyGameplayCommandsSystem.h"
#include "GameLogic/Systems/ApplyInputToEntitySystem.h"
#include "GameLogic/Systems/CharacterStateSystem.h"
#include "GameLogic/Systems/ClientInpuntSendSystem.h"
#include "GameLogic/Systems/ClientNetworkSystem.h"
#include "GameLogic/Systems/ControlSystem.h"
#include "GameLogic/Systems/DeadEntitiesDestructionSystem.h"
#include "GameLogic/Systems/DebugDrawSystem.h"
#include "GameLogic/Systems/FetchClientInputFromHistorySystem.h"
#include "GameLogic/Systems/FetchConfirmedCommandsSystem.h"
#include "GameLogic/Systems/InputSystem.h"
#include "GameLogic/Systems/MovementSystem.h"
#include "GameLogic/Systems/PopulateInputHistorySystem.h"
#include "GameLogic/Systems/RenderSystem.h"
#include "GameLogic/Systems/ResourceStreamingSystem.h"
#include "GameLogic/Systems/SaveCommandsToHistorySystem.h"
#include "GameLogic/Systems/SaveMovementToHistorySystem.h"

#ifdef IMGUI_ENABLED
#include "GameLogic/Systems/ImguiSystem.h"
#endif // IMGUI_ENABLED
#include "GameLogic/Initialization/StateMachines.h"

void TankClientGame::preStart(const ArgumentsParser& arguments, RenderAccessorGameRef renderAccessor)
{
	SCOPED_PROFILER("TankClientGame::preStart");

	ComponentsRegistration::RegisterComponents(getComponentFactory());
	ComponentsRegistration::RegisterJsonSerializers(getComponentSerializers());

	getWorldHolder().setWorld(mGameStateRewinder.getWorld(mGameStateRewinder.getTimeData().lastFixedUpdateIndex));

	std::optional<HAL::ConnectionManager::NetworkAddress> newNetworkAddress = HAL::ConnectionManager::NetworkAddress::FromString(arguments.getArgumentValue("connect", "127.0.0.1:14436"));
	if (newNetworkAddress.has_value())
	{
		mServerAddress = *newNetworkAddress;
	}

	initSystems();

	GameDataLoader::LoadWorld(getWorldHolder().getWorld(), arguments.getArgumentValue("world", "test"), getComponentSerializers());
	GameDataLoader::LoadGameData(getGameData(), arguments.getArgumentValue("gameData", "gameData"), getComponentSerializers());

	RenderAccessorComponent* renderAccessorComponent = getGameData().getGameComponents().addComponent<RenderAccessorComponent>();
	renderAccessorComponent->setAccessor(renderAccessor);

	TimeComponent* timeComponent = getWorldHolder().getWorld().getWorldComponents().addComponent<TimeComponent>();
	timeComponent->setValue(&mGameStateRewinder.getTimeData());

	{
		ConnectionManagerComponent* connectionManager = getWorldHolder().getGameData().getGameComponents().addComponent<ConnectionManagerComponent>();
		connectionManager->setManagerPtr(&mConnectionManager);
	}

	GameplayCommandFactoryComponent* gameplayFactory = mGameStateRewinder.getNotRewindableComponents().addComponent<GameplayCommandFactoryComponent>();
	Network::RegisterGameplayCommands(gameplayFactory->getInstanceRef());

	Game::preStart(arguments);

	if (HAL::Engine* engine = getEngine())
	{
		engine->init(this, &getInputData());
	}
}

void TankClientGame::initResources()
{
	SCOPED_PROFILER("TankGameClient::initResources");
	getResourceManager().loadAtlasesData("resources/atlas/atlas-list.json");
	Game::initResources();
}

void TankClientGame::dynamicTimePreFrameUpdate(float dt, int plannedFixedTimeUpdates)
{
	SCOPED_PROFILER("TankClientGame::dynamicTimePreFrameUpdate");
	Game::dynamicTimePreFrameUpdate(dt, plannedFixedTimeUpdates);

	mConnectionManager.processNetworkEvents();

	processCorrections();
}

void TankClientGame::fixedTimeUpdate(float dt)
{
	SCOPED_PROFILER("TankClientGame::fixedTimeUpdate");

	const auto [time] = getWorldHolder().getWorld().getWorldComponents().getComponents<const TimeComponent>();
	const u32 thisUpdateIdx = time->getValue()->lastFixedUpdateIndex + 1;
	mGameStateRewinder.advanceSimulationToNextUpdate(thisUpdateIdx);
	getWorldHolder().setWorld(mGameStateRewinder.getWorld(thisUpdateIdx));

	Game::fixedTimeUpdate(dt);
}

void TankClientGame::dynamicTimePostFrameUpdate(float dt, int processedFixedUpdates)
{
	SCOPED_PROFILER("TankClientGame::dynamicTimePostFrameUpdate");

	Game::dynamicTimePostFrameUpdate(dt, processedFixedUpdates);
	if (mShouldQuitGameNextTick)
	{
		mShouldQuitGame = true;
	}

	auto [clientGameData] = getWorldHolder().getWorld().getWorldComponents().getComponents<ClientGameDataComponent>();
	if (clientGameData != nullptr)
	{
		ConnectionId connectionId = clientGameData->getClientConnectionId();
		mConnectionManager.flushMesssagesForServerConnection(connectionId);
	}
}

TimeData& TankClientGame::getTimeData()
{
	return mGameStateRewinder.getTimeData();
}

void TankClientGame::initSystems()
{
	SCOPED_PROFILER("TankClientGame::initSystems");

	AssertFatal(getEngine(), "TankClientGame created without Engine. We're going to crash");

	getPreFrameSystemsManager().registerSystem<InputSystem>(getWorldHolder(), getInputData());
	getPreFrameSystemsManager().registerSystem<PopulateInputHistorySystem>(getWorldHolder(), mGameStateRewinder);
	getPreFrameSystemsManager().registerSystem<ClientInputSendSystem>(getWorldHolder(), mGameStateRewinder);
	getPreFrameSystemsManager().registerSystem<ClientNetworkSystem>(getWorldHolder(), mGameStateRewinder, mServerAddress, mShouldQuitGameNextTick);
	getGameLogicSystemsManager().registerSystem<FetchConfirmedCommandsSystem>(getWorldHolder(), mGameStateRewinder);
	getGameLogicSystemsManager().registerSystem<FetchClientInputFromHistorySystem>(getWorldHolder(), mGameStateRewinder);
	getGameLogicSystemsManager().registerSystem<ApplyConfirmedMovesSystem>(getWorldHolder(), mGameStateRewinder);
	getGameLogicSystemsManager().registerSystem<ApplyGameplayCommandsSystem>(getWorldHolder(), mGameStateRewinder);
	getGameLogicSystemsManager().registerSystem<ApplyInputToEntitySystem>(getWorldHolder());
	getGameLogicSystemsManager().registerSystem<ControlSystem>(getWorldHolder());
	getGameLogicSystemsManager().registerSystem<DeadEntitiesDestructionSystem>(getWorldHolder());
	getGameLogicSystemsManager().registerSystem<MovementSystem>(getWorldHolder());
	getGameLogicSystemsManager().registerSystem<CharacterStateSystem>(getWorldHolder());
	getGameLogicSystemsManager().registerSystem<AnimationSystem>(getWorldHolder());
	getGameLogicSystemsManager().registerSystem<SaveCommandsToHistorySystem>(getWorldHolder(), mGameStateRewinder);
	getGameLogicSystemsManager().registerSystem<SaveMovementToHistorySystem>(getWorldHolder(), mGameStateRewinder);
	getPostFrameSystemsManager().registerSystem<ResourceStreamingSystem>(getWorldHolder(), getResourceManager());
	getPostFrameSystemsManager().registerSystem<RenderSystem>(getWorldHolder(), getResourceManager(), getThreadPool());
	getPostFrameSystemsManager().registerSystem<DebugDrawSystem>(getWorldHolder(), mGameStateRewinder, getResourceManager());

#ifdef IMGUI_ENABLED
	getPostFrameSystemsManager().registerSystem<ImguiSystem>(mImguiDebugData, *getEngine());
#endif // IMGUI_ENABLED
}

void TankClientGame::correctUpdates(u32 firstUpdateToResimulateIdx)
{
	SCOPED_PROFILER("TankClientGame::correctUpdates");

	World& world = getWorldHolder().getWorld();

	// don't store references to component data, since the components will be destroyed during history rewinding
	const TimeData lastUpdateTime = *std::get<0>(world.getWorldComponents().getComponents<const TimeComponent>())->getValue();
	const float fixedUpdateDt = lastUpdateTime.lastFixedUpdateDt;

	LogInfo("Correct client updates from %u to %u", firstUpdateToResimulateIdx, lastUpdateTime.lastFixedUpdateIndex);

	AssertFatal(firstUpdateToResimulateIdx <= lastUpdateTime.lastFixedUpdateIndex, "We can't correct updates from the future");
	const u32 lastUpdateToResimulateIdx = lastUpdateTime.lastFixedUpdateIndex;

	// unwind the history back
	mGameStateRewinder.unwindBackInHistory(firstUpdateToResimulateIdx);

	// resimulate the frames starting with the diverged one
	for (u32 updateIdx = firstUpdateToResimulateIdx; updateIdx <= lastUpdateToResimulateIdx; ++updateIdx)
	{
		// this adds a new frame to the history
		fixedTimeUpdate(fixedUpdateDt);
	}
}

void TankClientGame::processCorrections()
{
	SCOPED_PROFILER("TankGameClient::processCorrections");

	if (mGameStateRewinder.getTimeData().lastFixedUpdateIndex == 0 || mGameStateRewinder.getTimeData().lastFixedUpdateIndex == std::numeric_limits<u32>::max())
	{
		return;
	}

	// local scope because after the corrections the values can change
	{
		const u32 firstUpdateIdx = mGameStateRewinder.getFirstStoredUpdateIdx();
		u32 firstDesyncedUpdate = mGameStateRewinder.getFirstDesyncedUpdateIdx();

		if (firstDesyncedUpdate == firstUpdateIdx)
		{
			// we can't correct the first update, since we don't have the previous state
			firstDesyncedUpdate += 1;
		}

		// if we need to process corrections
		if (firstDesyncedUpdate + 1 >= firstUpdateIdx && firstDesyncedUpdate != std::numeric_limits<u32>::max())
		{
			correctUpdates(firstDesyncedUpdate);
		}
	}

	removeOldUpdates();
}

void TankClientGame::removeOldUpdates()
{
	SCOPED_PROFILER("TankGameClient::removeOldUpdates");

	constexpr size_t MAX_STORED_UPDATES_COUNT = 60;

	World& world = getWorldHolder().getWorld();

	const TimeData lastUpdateTime = *std::get<0>(world.getWorldComponents().getComponents<const TimeComponent>())->getValue();

	const u32 lastUpdateIdx = lastUpdateTime.lastFixedUpdateIndex;
	const u32 firstUpdateIdx = mGameStateRewinder.getFirstStoredUpdateIdx();
	// for this update we can be sure that server won't do any corrections, but we may still be missing moves for it
	const u32 lastFullyConfirmedUpdateIdx = mGameStateRewinder.getLastConfirmedClientUpdateIdx(true);

	if (lastUpdateIdx == std::numeric_limits<u32>::max() || lastFullyConfirmedUpdateIdx == std::numeric_limits<u32>::max())
	{
		return;
	}

	const size_t updatesCountBeforeTrim = lastUpdateIdx - firstUpdateIdx + 1;
	// don't keep more frames than max
	const size_t maxUpdateToStore = std::min(updatesCountBeforeTrim, MAX_STORED_UPDATES_COUNT);
	// if we have some confirmed frames that are now safe to remove
	const u32 firstUpdateIdxToKeep = std::max(lastFullyConfirmedUpdateIdx, static_cast<u32>(lastUpdateIdx + 1 - maxUpdateToStore));

	mGameStateRewinder.trimOldFrames(firstUpdateIdxToKeep);
}

#endif // !DEDICATED_SERVER