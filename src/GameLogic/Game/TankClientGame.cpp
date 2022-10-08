#include "Base/precomp.h"

#ifndef DEDICATED_SERVER

#include "GameLogic/Game/TankClientGame.h"

#include <unordered_map>

#include "Base/Types/TemplateHelpers.h"

#include "GameData/ComponentRegistration/ComponentFactoryRegistration.h"
#include "GameData/ComponentRegistration/ComponentJsonSerializerRegistration.h"

#include "GameData/ComponentRegistration/ComponentFactoryRegistration.h"
#include "GameData/ComponentRegistration/ComponentJsonSerializerRegistration.h"

#include "GameData/Components/ClientGameDataComponent.generated.h"
#include "GameData/Components/ClientMovesHistoryComponent.generated.h"
#include "GameData/Components/ConnectionManagerComponent.generated.h"
#include "GameData/Components/GameplayCommandFactoryComponent.generated.h"
#include "GameData/Components/GameplayCommandHistoryComponent.generated.h"
#include "GameData/Components/GameplayCommandsComponent.generated.h"
#include "GameData/Components/GameplayInputComponent.generated.h"
#include "GameData/Components/InputHistoryComponent.generated.h"
#include "GameData/Components/MovementComponent.generated.h"
#include "GameData/Components/RenderAccessorComponent.generated.h"
#include "GameData/Components/TimeComponent.generated.h"
#include "GameData/Components/TransformComponent.generated.h"

#include "Utils/Application/ArgumentsParser.h"
#include "Utils/Network/GameplayCommands/GameplayCommandFactoryRegistration.h"
#include "Utils/Network/Messages/WorldSnapshotMessage.h"
#include "Utils/ResourceManagement/ResourceManager.h"
#include "Utils/World/GameDataLoader.h"

#include "HAL/Base/Engine.h"

#include "GameLogic/Systems/AnimationSystem.h"
#include "GameLogic/Systems/ApplyGameplayCommandsSystem.h"
#include "GameLogic/Systems/ApplyInputToEntitySystem.h"
#include "GameLogic/Systems/CharacterStateSystem.h"
#include "GameLogic/Systems/ClientInpuntSendSystem.h"
#include "GameLogic/Systems/ClientNetworkSystem.h"
#include "GameLogic/Systems/ControlSystem.h"
#include "GameLogic/Systems/DeadEntitiesDestructionSystem.h"
#include "GameLogic/Systems/DebugDrawSystem.h"
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

	std::optional<HAL::ConnectionManager::NetworkAddress> newNetworkAddress = HAL::ConnectionManager::NetworkAddress::FromString(arguments.getArgumentValue("connect", "127.0.0.1:14436"));
	if (newNetworkAddress.has_value())
	{
		mServerAddress = *newNetworkAddress;
	}

	initSystems();

	GameDataLoader::LoadWorld(getWorldHolder().getWorld(), arguments.getArgumentValue("world", "test"), getComponentSerializers());
	GameDataLoader::LoadGameData(getGameData(), arguments.getArgumentValue("gameData", "gameData"), getComponentSerializers());

	RenderAccessorComponent* renderAccessorComponent = getGameData().getGameComponents().getOrAddComponent<RenderAccessorComponent>();
	renderAccessorComponent->setAccessor(renderAccessor);

	{
		ConnectionManagerComponent* connectionManager = getWorldHolder().getGameData().getGameComponents().getOrAddComponent<ConnectionManagerComponent>();
		connectionManager->setManagerPtr(&mConnectionManager);
	}

	ClientMovesHistoryComponent* clientMovesHistory = getWorldHolder().getWorld().getNotRewindableWorldComponents().getOrAddComponent<ClientMovesHistoryComponent>();
	clientMovesHistory->getDataRef().updates.emplace_back(); // emplace empty moves before the first frame, to be able to resimulate it

	GameplayCommandFactoryComponent* gameplayFactory = getWorldHolder().getWorld().getNotRewindableWorldComponents().getOrAddComponent<GameplayCommandFactoryComponent>();
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

	processMoveCorrections();
}

void TankClientGame::fixedTimeUpdate(float dt)
{
	SCOPED_PROFILER("TankClientGame::fixedTimeUpdate");

	getWorldHolder().getWorld().addNewFrameToTheHistory();

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

void TankClientGame::initSystems()
{
	SCOPED_PROFILER("TankClientGame::initSystems");

	AssertFatal(getEngine(), "TankClientGame created without Engine. We're going to crash");

	getPreFrameSystemsManager().registerSystem<InputSystem>(getWorldHolder(), getInputData());
	getPreFrameSystemsManager().registerSystem<PopulateInputHistorySystem>(getWorldHolder());
	getPreFrameSystemsManager().registerSystem<ClientInputSendSystem>(getWorldHolder());
	getPreFrameSystemsManager().registerSystem<ClientNetworkSystem>(getWorldHolder(), mServerAddress, mShouldQuitGameNextTick);
	getGameLogicSystemsManager().registerSystem<ApplyGameplayCommandsSystem>(getWorldHolder());
	getGameLogicSystemsManager().registerSystem<ApplyInputToEntitySystem>(getWorldHolder());
	getGameLogicSystemsManager().registerSystem<ControlSystem>(getWorldHolder());
	getGameLogicSystemsManager().registerSystem<DeadEntitiesDestructionSystem>(getWorldHolder());
	getGameLogicSystemsManager().registerSystem<MovementSystem>(getWorldHolder());
	getGameLogicSystemsManager().registerSystem<CharacterStateSystem>(getWorldHolder());
	getGameLogicSystemsManager().registerSystem<AnimationSystem>(getWorldHolder());
	getGameLogicSystemsManager().registerSystem<SaveCommandsToHistorySystem>(getWorldHolder());
	getGameLogicSystemsManager().registerSystem<SaveMovementToHistorySystem>(getWorldHolder());
	getPostFrameSystemsManager().registerSystem<ResourceStreamingSystem>(getWorldHolder(), getResourceManager());
	getPostFrameSystemsManager().registerSystem<RenderSystem>(getWorldHolder(), getResourceManager(), getThreadPool());
	getPostFrameSystemsManager().registerSystem<DebugDrawSystem>(getWorldHolder(), getResourceManager());

#ifdef IMGUI_ENABLED
	getPostFrameSystemsManager().registerSystem<ImguiSystem>(mImguiDebugData, *getEngine());
#endif // IMGUI_ENABLED
}

void TankClientGame::correctUpdates(u32 lastUpdateIdxWithAuthoritativeMoves, bool overrideState)
{
	SCOPED_PROFILER("TankClientGame::correctUpdates");

	World& world = getWorldHolder().getWorld();

	// don't store references to component data, since the components will be destroyed during history rewinding
	const TimeData lastUpdateTime = std::get<0>(world.getWorldComponents().getComponents<const TimeComponent>())->getValue();
	const float fixedUpdateDt = lastUpdateTime.lastFixedUpdateDt;

	LogInfo("Correct client updates from %u to %u", lastUpdateIdxWithAuthoritativeMoves + 1, lastUpdateTime.lastFixedUpdateIndex);

	AssertFatal(lastUpdateIdxWithAuthoritativeMoves <= lastUpdateTime.lastFixedUpdateIndex, "We can't correct updates from the future");
	const u32 framesToResimulate = lastUpdateTime.lastFixedUpdateIndex - lastUpdateIdxWithAuthoritativeMoves;

	auto [clientMovesHistory] = world.getNotRewindableWorldComponents().getComponents<ClientMovesHistoryComponent>();
	MovementHistory& movesHistory = clientMovesHistory->getDataRef();
	std::vector<MovementUpdateData>& updates = movesHistory.updates;

	// unwind the history back
	world.unwindBackInHistory(overrideState ? framesToResimulate - 1 : framesToResimulate);
	updates.erase(updates.begin() + (updates.size() - framesToResimulate), updates.end());
	movesHistory.lastUpdateIdx -= framesToResimulate;

	// apply moves to the diverged frame
	std::unordered_map<Entity, EntityMoveData> entityMoves;
	for (const EntityMoveData& move : updates.back().moves)
	{
		entityMoves.emplace(move.entity, move);
	}

	if (overrideState)
	{
		clearFrameState(world);
	}

	world.getEntityManager().forEachComponentSetWithEntity<MovementComponent, TransformComponent>(
		[&entityMoves](Entity entity, MovementComponent* movement, TransformComponent* transform)
	{
		const auto it = entityMoves.find(entity);

		if (it == entityMoves.end())
		{
			return;
		}

		const EntityMoveData& move = it->second;

		transform->setLocation(move.location);
		movement->setUpdateTimestamp(move.timestamp);
	});

	// resimulate later frames
	InputHistoryComponent* inputHistory = world.getNotRewindableWorldComponents().getOrAddComponent<InputHistoryComponent>();
	GameplayCommandHistoryComponent* commandHistory = world.getNotRewindableWorldComponents().getOrAddComponent<GameplayCommandHistoryComponent>();
	const std::vector<GameplayInput::FrameState>& inputs = inputHistory->getInputs();
	AssertFatal(inputs.size() >= framesToResimulate, "Size of input history can't be less than size of move history");
	size_t inputHistoryIndexShift = inputs.size() - framesToResimulate;
	for (u32 i = 0; i < framesToResimulate; ++i)
	{
		const u32 previousFrameIndex = lastUpdateIdxWithAuthoritativeMoves + i;
		ComponentSetHolder& thisFrameWorldComponents = getWorldHolder().getWorld().getWorldComponents();

		GameplayInputComponent* gameplayInput = thisFrameWorldComponents.getOrAddComponent<GameplayInputComponent>();
		gameplayInput->setCurrentFrameState(inputs[i + inputHistoryIndexShift]);

		GameplayCommandsComponent* gameplayCommands = world.getWorldComponents().getOrAddComponent<GameplayCommandsComponent>();
		// if we have confirmed updates for this frame, apply them instead of what we generated last frame
		if (previousFrameIndex <= commandHistory->getLastCommandUpdateIdx() && previousFrameIndex >= commandHistory->getLastCommandUpdateIdx() - commandHistory->getRecords().size() + 1)
		{
			const size_t idx = previousFrameIndex - (commandHistory->getLastCommandUpdateIdx() - commandHistory->getRecords().size() + 1);
			gameplayCommands->setData(commandHistory->getRecords()[idx]);
		}

		// this adds a new frame to the history
		fixedTimeUpdate(fixedUpdateDt);
	}
}

void TankClientGame::processMoveCorrections()
{
	SCOPED_PROFILER("TankGameClient::processMoveCorrections");

	World& world = getWorldHolder().getWorld();

	auto [clientMovesHistory] = world.getNotRewindableWorldComponents().getComponents<ClientMovesHistoryComponent>();
	GameplayCommandHistoryComponent* commandHistory = world.getNotRewindableWorldComponents().getOrAddComponent<GameplayCommandHistoryComponent>();

	// local scope because after the corrections the values can change
	{
		const TimeData lastUpdateTime = std::get<0>(world.getWorldComponents().getComponents<const TimeComponent>())->getValue();

		const u32 lastUpdateIdx = lastUpdateTime.lastFixedUpdateIndex;
		const u32 firstUpdateIdx = lastUpdateIdx - world.getStoredFramesCount() + 1;
		const u32 lastConfirmedMovesUpdateIdx = clientMovesHistory->getData().lastConfirmedUpdateIdx;
		AssertFatal(lastConfirmedMovesUpdateIdx != clientMovesHistory->getData().updateIdxProducedDesyncedMoves, "We can't have the same frame to be confermed and desynced at the same time");
		const u32 desyncedMoveUpdateIdx = (clientMovesHistory->getData().updateIdxProducedDesyncedMoves > lastConfirmedMovesUpdateIdx) ? clientMovesHistory->getData().updateIdxProducedDesyncedMoves : std::numeric_limits<u32>::max();
		const u32 desyncedCommandUpdateIdx = commandHistory->getUpdateIdxProducedDesyncedCommands();
		const u32 firstDesyncedUpdate = std::min(desyncedMoveUpdateIdx, desyncedCommandUpdateIdx);

		// if we need to process corrections
		if (firstDesyncedUpdate + 1 >= firstUpdateIdx && firstDesyncedUpdate != std::numeric_limits<u32>::max())
		{
			const u32 updateIdxWithRewritingCommands = commandHistory->getUpdateIdxWithRewritingCommands() + 1;
			const bool shouldOverride = updateIdxWithRewritingCommands != std::numeric_limits<u32>::max() && updateIdxWithRewritingCommands - 1 >= firstUpdateIdx && updateIdxWithRewritingCommands - 1 <= lastUpdateIdx;
			const u32 firstUpdateToCorrect = shouldOverride ? updateIdxWithRewritingCommands - 1 : firstDesyncedUpdate;
			correctUpdates(firstUpdateToCorrect, shouldOverride);

			// mark the desynced update as confirmed since now we applied moves fror server to it
			clientMovesHistory->getDataRef().lastConfirmedUpdateIdx = firstDesyncedUpdate;
			clientMovesHistory->getDataRef().updateIdxProducedDesyncedMoves = std::numeric_limits<u32>::max();
			commandHistory->setUpdateIdxProducedDesyncedCommands(std::numeric_limits<u32>::max());
			commandHistory->setUpdateIdxWithRewritingCommands(std::numeric_limits<u32>::max());
		}
	}

	removeOldUpdates();
}

void TankClientGame::removeOldUpdates()
{
	SCOPED_PROFILER("TankGameClient::removeOldUpdates");

	constexpr size_t MAX_STORED_UPDATES_COUNT = 60;

	World& world = getWorldHolder().getWorld();

	auto [clientMovesHistory] = world.getNotRewindableWorldComponents().getComponents<ClientMovesHistoryComponent>();
	GameplayCommandHistoryComponent* commandHistory = world.getNotRewindableWorldComponents().getOrAddComponent<GameplayCommandHistoryComponent>();
	const TimeData lastUpdateTime = std::get<0>(world.getWorldComponents().getComponents<const TimeComponent>())->getValue();

	const MovementHistory& movementHistory = clientMovesHistory->getData();
	std::vector<MovementUpdateData>& movementUpdates = clientMovesHistory->getDataRef().updates;

	const u32 lastUpdateIdx = lastUpdateTime.lastFixedUpdateIndex;
	const u32 firstUpdateIdx = lastUpdateIdx - world.getStoredFramesCount() + 1;
	// for this update we can be sure that server won't do any corrections, but we may still be missing moves for it
	const u32 lastUpdateWithAllInputsConfirmed = commandHistory->getLastUpdateIdxWithAllPlayerInputsConfirmed();
	const u32 lastConfirmedUpdateIdx = clientMovesHistory->getData().lastConfirmedUpdateIdx;
	const u32 lastUpdateSafeToRemove = std::min(lastUpdateWithAllInputsConfirmed, lastConfirmedUpdateIdx);

	const size_t updatesCountBeforeTrim = lastUpdateIdx - firstUpdateIdx + 1;
	// don't keep more frames than max
	size_t updatesCountAfterTrim = std::min(updatesCountBeforeTrim, MAX_STORED_UPDATES_COUNT);
	// if we have some confirmed frames that are now safe to remove
	if (lastUpdateSafeToRemove >= firstUpdateIdx)
	{
		const size_t lastSafeToRemoveUpdateRecordIdx = lastUpdateSafeToRemove - firstUpdateIdx;
		updatesCountAfterTrim = std::min(updatesCountAfterTrim, updatesCountBeforeTrim - lastSafeToRemoveUpdateRecordIdx - 1);
	}
	const u32 firstUpdateToKeep = lastUpdateIdx - updatesCountAfterTrim + 1;

	// we keep one more frame for movement and command records because we use them to recalculate the next frame after
	{
		const u32 firstStoredUpdateIdx = movementHistory.lastUpdateIdx - movementUpdates.size() + 1;
		AssertFatal(firstUpdateToKeep >= firstStoredUpdateIdx + 1, "We can't have less movement records than stored frames");
		const size_t firstIndexToKeep = firstUpdateToKeep - firstStoredUpdateIdx - 1;
		movementUpdates.erase(movementUpdates.begin(), movementUpdates.begin() + firstIndexToKeep);
	}
	{
		const u32 firstStoredUpdateIdx = movementHistory.lastUpdateIdx - movementUpdates.size() + 1;
		if (firstUpdateToKeep < firstStoredUpdateIdx + 1)
		{
			const size_t firstIndexToKeep = firstUpdateToKeep - firstStoredUpdateIdx - 1;
			std::vector<Network::GameplayCommandList>& cmdHistoryRecords = commandHistory->getRecordsRef();
			cmdHistoryRecords.erase(cmdHistoryRecords.begin(), cmdHistoryRecords.begin() + firstIndexToKeep);
		}
	}

	world.trimOldFrames(updatesCountAfterTrim);
}

void TankClientGame::clearFrameState(World& world)
{
	Network::CleanBeforeApplyingSnapshot(world);
}

#endif // !DEDICATED_SERVER
