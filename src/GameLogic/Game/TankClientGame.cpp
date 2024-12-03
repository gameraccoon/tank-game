#include "EngineCommon/precomp.h"

#include "EngineCommon/TimeConstants.h"

#ifndef DEDICATED_SERVER

#include "EngineCommon/Types/String/ResourcePath.h"

#include "GameData/ComponentRegistration/ComponentFactoryRegistration.h"
#include "GameData/ComponentRegistration/ComponentJsonSerializerRegistration.h"
#include "GameData/Components/ClientGameDataComponent.generated.h"
#include "GameData/Components/ConnectionManagerComponent.generated.h"
#include "GameData/Components/GameplayCommandFactoryComponent.generated.h"
#include "GameData/Components/MoveInterpolationComponent.generated.h"
#include "GameData/Components/NetworkIdComponent.generated.h"
#include "GameData/Components/RenderAccessorComponent.generated.h"
#include "GameData/Components/TimeComponent.generated.h"
#include "GameData/Network/MovementHistory.h"

#include "HAL/Base/Engine.h"

#include "EngineUtils/Application/ArgumentsParser.h"
#include "EngineUtils/ResourceManagement/ResourceManager.h"

#include "GameUtils/Network/GameplayCommands/GameplayCommandFactoryRegistration.h"
#include "GameUtils/Network/GameStateRewinder.h"
#include "GameUtils/World/GameDataLoader.h"

#include "GameLogic/Game/TankClientGame.h"
#include "GameLogic/Systems/AnimationSystem.h"
#include "GameLogic/Systems/ApplyConfirmedMovesSystem.h"
#include "GameLogic/Systems/ApplyGameplayCommandsSystem.h"
#include "GameLogic/Systems/ApplyInputToEntitySystem.h"
#include "GameLogic/Systems/CharacterStateSystem.h"
#include "GameLogic/Systems/ClientInpuntSendSystem.h"
#include "GameLogic/Systems/ClientNetworkSystem.h"
#include "GameLogic/Systems/CollisionResolutionSystem.h"
#include "GameLogic/Systems/ControlSystem.h"
#include "GameLogic/Systems/DeadEntitiesDestructionSystem.h"
#include "GameLogic/Systems/DebugDrawSystem.h"
#include "GameLogic/Systems/DebugInputSystem.h"
#include "GameLogic/Systems/FetchClientInputFromHistorySystem.h"
#include "GameLogic/Systems/FetchConfirmedCommandsSystem.h"
#include "GameLogic/Systems/InputSystem.h"
#include "GameLogic/Systems/MovementSystem.h"
#include "GameLogic/Systems/PopulateInputHistorySystem.h"
#include "GameLogic/Systems/ProjectileLifetimeSystem.h"
#include "GameLogic/Systems/RenderSystem.h"
#include "GameLogic/Systems/ResourceStreamingSystem.h"
#include "GameLogic/Systems/SaveCommandsToHistorySystem.h"
#include "GameLogic/Systems/SaveMovementToHistorySystem.h"
#include "GameLogic/Systems/ShootingSystem.h"

#ifdef IMGUI_ENABLED
#include "GameLogic/Systems/ImguiSystem.h"
#endif // IMGUI_ENABLED
#include "GameLogic/Initialization/StateMachines.h"

void TankClientGame::preStart(const ArgumentsParser& arguments, std::optional<RenderAccessorGameRef> renderAccessor)
{
	SCOPED_PROFILER("TankClientGame::preStart");

	ComponentsRegistration::RegisterComponents(getComponentFactory());
	ComponentsRegistration::RegisterJsonSerializers(getComponentSerializers());

	getWorldHolder().setDynamicWorld(mGameStateRewinder.getDynamicWorld(mGameStateRewinder.getTimeData().lastFixedUpdateIndex));

	const std::optional<HAL::Network::NetworkAddress> newNetworkAddress = HAL::Network::NetworkAddress::FromString(arguments.getArgumentValue("connect").value_or("127.0.0.1:14436"));
	if (newNetworkAddress.has_value())
	{
		mServerAddress = *newNetworkAddress;
	}

	initSystems();

	GameDataLoader::LoadWorld(getWorldHolder().getStaticWorldLayer(), std::filesystem::current_path(), arguments.getArgumentValue("world").value_or("test"), getComponentSerializers());
	GameDataLoader::LoadGameData(getGameData(), std::filesystem::current_path(), arguments.getArgumentValue("gameData").value_or("gameData"), getComponentSerializers());

	RenderAccessorComponent* renderAccessorComponent = getGameData().getGameComponents().addComponent<RenderAccessorComponent>();
	renderAccessorComponent->setAccessor(renderAccessor);

	TimeComponent* timeComponent = getWorldHolder().getDynamicWorldLayer().getWorldComponents().addComponent<TimeComponent>();
	timeComponent->setValue(&mGameStateRewinder.getTimeData());

	{
		ConnectionManagerComponent* connectionManager = getWorldHolder().getGameData().getGameComponents().addComponent<ConnectionManagerComponent>();
		connectionManager->setManagerPtr(&mConnectionManager);
	}

	GameplayCommandFactoryComponent* gameplayFactory = mGameStateRewinder.getNotRewindableComponents().addComponent<GameplayCommandFactoryComponent>();
	RegisterGameplayCommands(gameplayFactory->getInstanceRef());

	Game::preStart(arguments);

#ifndef DISABLE_SDL
	if (HAL::Engine* engine = getEngine())
	{
		engine->init(this, &getInputData());
	}
#endif // !DISABLE_SDL
}

void TankClientGame::initResources()
{
	SCOPED_PROFILER("TankGameClient::initResources");
	getResourceManager().loadAtlasesData(RelativeResourcePath("resources/atlas/atlas-list.json"));
	Game::initResources();
}

void TankClientGame::dynamicTimePreFrameUpdate(const float dt, const int plannedFixedTimeUpdates)
{
	SCOPED_PROFILER("TankClientGame::dynamicTimePreFrameUpdate");
	Game::dynamicTimePreFrameUpdate(dt, plannedFixedTimeUpdates);

	mConnectionManager.processNetworkEvents();

	processCorrections();
}

void TankClientGame::fixedTimeUpdate(const float dt)
{
	SCOPED_PROFILER("TankClientGame::fixedTimeUpdate");

	const auto [time] = getWorldHolder().getDynamicWorldLayer().getWorldComponents().getComponents<const TimeComponent>();
	const u32 thisUpdateIdx = time->getValue()->lastFixedUpdateIndex + 1;
	mGameStateRewinder.advanceSimulationToNextUpdate(thisUpdateIdx);
	getWorldHolder().setDynamicWorld(mGameStateRewinder.getDynamicWorld(thisUpdateIdx));

	Game::fixedTimeUpdate(dt);

	mFrameTimeCorrector.advanceOneUpdate();
}

void TankClientGame::dynamicTimePostFrameUpdate(const float dt, const int processedFixedTimeUpdates)
{
	SCOPED_PROFILER("TankClientGame::dynamicTimePostFrameUpdate");

	Game::dynamicTimePostFrameUpdate(dt, processedFixedTimeUpdates);
	if (mShouldQuitGameNextTick)
	{
		mShouldQuitGame = true;
	}

	auto [clientGameData] = getWorldHolder().getDynamicWorldLayer().getWorldComponents().getComponents<ClientGameDataComponent>();
	if (clientGameData != nullptr)
	{
		const ConnectionId connectionId = clientGameData->getClientConnectionId();
		mConnectionManager.flushMessagesForServerConnection(connectionId);
	}
}

std::chrono::duration<int64_t, std::micro> TankClientGame::getFrameLengthCorrection() const
{
	return mFrameTimeCorrector.getFrameLengthCorrection();
}

TimeData& TankClientGame::getTimeData()
{
	return mGameStateRewinder.getTimeData();
}

void TankClientGame::initSystems()
{
	SCOPED_PROFILER("TankClientGame::initSystems");

#ifndef DISABLE_SDL
	getNotPausablePreFrameSystemsManager().registerSystem<DebugInputSystem>(getWorldHolder(), getInputData(), mShouldPauseGame);
#endif // !DISABLE_SDL

#ifndef DISABLE_SDL
	getPreFrameSystemsManager().registerSystem<InputSystem>(getWorldHolder(), getInputData());
#endif // !DISABLE_SDL
	getPreFrameSystemsManager().registerSystem<PopulateInputHistorySystem>(getWorldHolder(), mGameStateRewinder);
	getPreFrameSystemsManager().registerSystem<ClientInputSendSystem>(getWorldHolder(), mGameStateRewinder);
	getPreFrameSystemsManager().registerSystem<ClientNetworkSystem>(getWorldHolder(), mGameStateRewinder, mServerAddress, mFrameTimeCorrector, mShouldQuitGameNextTick);

	getGameLogicSystemsManager().registerSystem<FetchConfirmedCommandsSystem>(getWorldHolder(), mGameStateRewinder);
	getGameLogicSystemsManager().registerSystem<FetchClientInputFromHistorySystem>(getWorldHolder(), mGameStateRewinder);
	getGameLogicSystemsManager().registerSystem<ApplyConfirmedMovesSystem>(getWorldHolder(), mGameStateRewinder);
	getGameLogicSystemsManager().registerSystem<ApplyGameplayCommandsSystem>(getWorldHolder(), mGameStateRewinder);
	getGameLogicSystemsManager().registerSystem<ApplyInputToEntitySystem>(getWorldHolder());
	getGameLogicSystemsManager().registerSystem<ControlSystem>(getWorldHolder());
	getGameLogicSystemsManager().registerSystem<ProjectileLifetimeSystem>(getWorldHolder());
	getGameLogicSystemsManager().registerSystem<DeadEntitiesDestructionSystem>(getWorldHolder());
	getGameLogicSystemsManager().registerSystem<CharacterStateSystem>(getWorldHolder());
	getGameLogicSystemsManager().registerSystem<MovementSystem>(getWorldHolder());
	getGameLogicSystemsManager().registerSystem<CollisionResolutionSystem>(getWorldHolder());
	getGameLogicSystemsManager().registerSystem<ShootingSystem>(getWorldHolder());
	getGameLogicSystemsManager().registerSystem<AnimationSystem>(getWorldHolder());
	getGameLogicSystemsManager().registerSystem<SaveCommandsToHistorySystem>(getWorldHolder(), mGameStateRewinder);
	getGameLogicSystemsManager().registerSystem<SaveMovementToHistorySystem>(getWorldHolder(), mGameStateRewinder);

	getNotPausablePostFrameSystemsManager().registerSystem<ResourceStreamingSystem>(getWorldHolder(), getResourceManager());

#ifndef DISABLE_SDL
	getNotPausablePostFrameSystemsManager().registerSystem<RenderSystem>(getWorldHolder(), getResourceManager());
	getNotPausablePostFrameSystemsManager().registerSystem<DebugDrawSystem>(getWorldHolder(), mGameStateRewinder, getResourceManager());

#if defined(IMGUI_ENABLED)
	if (HAL::Engine* engine = getEngine())
	{
		getNotPausablePostFrameSystemsManager().registerSystem<ImguiSystem>(mImguiDebugData, *engine);
	}
#endif // IMGUI_ENABLED
#endif // !DISABLE_SDL
}

void TankClientGame::correctUpdates(u32 firstUpdateToResimulateIdx)
{
	SCOPED_PROFILER("TankClientGame::correctUpdates");

	WorldLayer& world = getWorldHolder().getDynamicWorldLayer();

	// don't store references to component data, since the components will be destroyed during history rewinding
	const TimeData lastUpdateTime = *std::get<0>(world.getWorldComponents().getComponents<const TimeComponent>())->getValue();
	const float fixedUpdateDt = lastUpdateTime.lastFixedUpdateDt;

	// save moves from the incorrect history version
	MovementUpdateData copyOfOldMoves = mGameStateRewinder.getMovesForUpdate(lastUpdateTime.lastFixedUpdateIndex);

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

	// set the moves from the old history version for interpolation
	{
		SCOPED_PROFILER("UpdateInterpolationData");
		constexpr int INTERPOLATION_UPDATES = static_cast<int>(0.2f / TimeConstants::ONE_FIXED_UPDATE_SEC);
		constexpr float NO_INTERPOLATION_DISTANCE = 1.5f;
		constexpr float NO_INTERPOLATION_DISTANCE_SQ = NO_INTERPOLATION_DISTANCE * NO_INTERPOLATION_DISTANCE;
		getWorldHolder().getDynamicWorldLayer().getEntityManager().forEachComponentSet<MoveInterpolationComponent, const NetworkIdComponent>([&copyOfOldMoves, lastUpdateTime](MoveInterpolationComponent* moveInterpolation, const NetworkIdComponent* networkId) {
			const NetworkEntityId id = networkId->getId();
			// this is quite expensive, but we assume we are not doing this often, and there are not many moving entities
			const auto it = std::ranges::find_if(copyOfOldMoves.moves, [id](const EntityMoveData& moveData) {
				return moveData.networkEntityId == id;
			});

			if (it != copyOfOldMoves.moves.end() && (it->location - moveInterpolation->getOriginalPosition()).qSize() > NO_INTERPOLATION_DISTANCE_SQ)
			{
				moveInterpolation->setOriginalPosition(it->location);
				moveInterpolation->setOriginalTimestamp(lastUpdateTime.lastFixedUpdateTimestamp);
				moveInterpolation->setTargetTimestamp(lastUpdateTime.lastFixedUpdateTimestamp.getIncreasedByUpdateCount(INTERPOLATION_UPDATES));
			}
		});
	}
}

void TankClientGame::processCorrections()
{
	SCOPED_PROFILER("TankGameClient::processCorrections");

	if (mGameStateRewinder.getTimeData().lastFixedUpdateIndex == 0 || mGameStateRewinder.getTimeData().lastFixedUpdateIndex == GameStateRewinder::INVALID_UPDATE_IDX)
	{
		return;
	}

	// local scope because after the corrections the values can change
	{
		const u32 firstUpdateIdx = mGameStateRewinder.getFirstStoredUpdateIdx();
		u32 firstDesyncedUpdate = mGameStateRewinder.getFirstDesyncedUpdateIdx();

		if (firstDesyncedUpdate == firstUpdateIdx)
		{
			ReportError("We tried to correct the first update that we store, we can't do that");
			// we can't correct the first update, since we don't have the previous state
			firstDesyncedUpdate += 1;
		}

		// if the desynced update in the past
		if (firstDesyncedUpdate <= mGameStateRewinder.getTimeData().lastFixedUpdateIndex)
		{
			// if we need to process corrections
			if (firstDesyncedUpdate + 1 >= firstUpdateIdx && firstDesyncedUpdate != GameStateRewinder::INVALID_UPDATE_IDX)
			{
				correctUpdates(firstDesyncedUpdate);
			}
		}
	}

	removeOldUpdates();
}

void TankClientGame::removeOldUpdates()
{
	SCOPED_PROFILER("TankGameClient::removeOldUpdates");

	constexpr size_t MAX_STORED_UPDATES_COUNT = 60;

	WorldLayer& world = getWorldHolder().getDynamicWorldLayer();

	const auto [time] = world.getWorldComponents().getComponents<const TimeComponent>();
	const TimeData& lastUpdateTime = *time->getValue();

	const u32 lastUpdateIdx = lastUpdateTime.lastFixedUpdateIndex;
	const u32 firstUpdateIdx = mGameStateRewinder.getFirstStoredUpdateIdx();
	// for this update we can be sure that server won't do any corrections, but we may still be missing moves for it
	const u32 lastFullyConfirmedUpdateIdx = mGameStateRewinder.getLastConfirmedClientUpdateIdx();

	if (lastUpdateIdx == std::numeric_limits<u32>::max() || lastFullyConfirmedUpdateIdx == GameStateRewinder::INVALID_UPDATE_IDX)
	{
		return;
	}

	const size_t updatesCountBeforeTrim = lastUpdateIdx - firstUpdateIdx + 1;
	// don't keep more frames than max
	const size_t maxUpdateToStore = std::min(updatesCountBeforeTrim, MAX_STORED_UPDATES_COUNT);
	// if we have some confirmed frames that are now safe to remove
	const u32 minBoundOfFramesToKeep = static_cast<u32>(lastUpdateIdx + 1 - maxUpdateToStore);
	const u32 maxBoundOfFramesToKeep = lastUpdateIdx - 1;
	AssertFatal(minBoundOfFramesToKeep <= maxBoundOfFramesToKeep, "Invalid bounds of frames");
	const u32 firstUpdateIdxToKeep = std::clamp(lastFullyConfirmedUpdateIdx, minBoundOfFramesToKeep, maxBoundOfFramesToKeep);

	mGameStateRewinder.trimOldUpdates(firstUpdateIdxToKeep);
}

#endif // !DEDICATED_SERVER
