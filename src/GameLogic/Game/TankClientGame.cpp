#include "Base/precomp.h"

#include "GameLogic/Game/TankClientGame.h"

#include <unordered_map>

#include "Base/Types/TemplateHelpers.h"

#include "GameData/ComponentRegistration/ComponentFactoryRegistration.h"
#include "GameData/ComponentRegistration/ComponentJsonSerializerRegistration.h"

#include "GameData/ComponentRegistration/ComponentFactoryRegistration.h"
#include "GameData/ComponentRegistration/ComponentJsonSerializerRegistration.h"

#include "GameData/Components/CharacterStateComponent.generated.h"
#include "GameData/Components/ClientGameDataComponent.generated.h"
#include "GameData/Components/ClientMovesHistoryComponent.generated.h"
#include "GameData/Components/ConnectionManagerComponent.generated.h"
#include "GameData/Components/GameplayInputComponent.generated.h"
#include "GameData/Components/InputHistoryComponent.generated.h"
#include "GameData/Components/MovementComponent.generated.h"
#include "GameData/Components/NetworkIdComponent.generated.h"
#include "GameData/Components/NetworkIdMappingComponent.generated.h"
#include "GameData/Components/RenderAccessorComponent.generated.h"
#include "GameData/Components/SpriteCreatorComponent.generated.h"
#include "GameData/Components/TimeComponent.generated.h"
#include "GameData/Components/TransformComponent.generated.h"

#include "Utils/Application/ArgumentsParser.h"
#include "Utils/ResourceManagement/ResourceManager.h"
#include "Utils/World/GameDataLoader.h"

#include "HAL/Base/Engine.h"

#include "GameLogic/Systems/AnimationSystem.h"
#include "GameLogic/Systems/CharacterStateSystem.h"
#include "GameLogic/Systems/ClientInpuntSendSystem.h"
#include "GameLogic/Systems/ClientNetworkSystem.h"
#include "GameLogic/Systems/ControlSystem.h"
#include "GameLogic/Systems/DeadEntitiesDestructionSystem.h"
#include "GameLogic/Systems/DebugDrawSystem.h"
#include "GameLogic/Systems/InputSystem.h"
#include "GameLogic/Systems/MovementSystem.h"
#include "GameLogic/Systems/RenderSystem.h"
#include "GameLogic/Systems/ResourceStreamingSystem.h"

#ifdef IMGUI_ENABLED
#include "GameLogic/Systems/ImguiSystem.h"
#endif // IMGUI_ENABLED
#include "GameLogic/Initialization/StateMachines.h"

void TankClientGame::preStart(ArgumentsParser& arguments, RenderAccessorGameRef renderAccessor)
{
	SCOPED_PROFILER("TankClientGame::preStart");

	ComponentsRegistration::RegisterComponents(getComponentFactory());
	ComponentsRegistration::RegisterJsonSerializers(getComponentSerializers());

	initSystems();

	GameDataLoader::LoadWorld(getWorldHolder().getWorld(), arguments.getArgumentValue("world", "test"), getComponentSerializers());
	GameDataLoader::LoadGameData(getGameData(), arguments.getArgumentValue("gameData", "gameData"), getComponentSerializers());

	RenderAccessorComponent* renderAccessorComponent = getGameData().getGameComponents().getOrAddComponent<RenderAccessorComponent>();
	renderAccessorComponent->setAccessor(renderAccessor);

	EntityManager& worldEntityManager = getWorldHolder().getWorld().getEntityManager();
	Entity controlledEntity = worldEntityManager.addEntity();
	{
		TransformComponent* transform = worldEntityManager.addComponent<TransformComponent>(controlledEntity);
		transform->setLocation(Vector2D(50, 50));
		MovementComponent* movement = worldEntityManager.addComponent<MovementComponent>(controlledEntity);
		movement->setOriginalSpeed(20.0f);
		worldEntityManager.addComponent<InputHistoryComponent>(controlledEntity);
		SpriteCreatorComponent* spriteCreator = worldEntityManager.addComponent<SpriteCreatorComponent>(controlledEntity);
		spriteCreator->getDescriptionsRef().emplace_back(SpriteParams{Vector2D(16,16), ZERO_VECTOR}, "resources/textures/tank-enemy-level1-1.png");
		worldEntityManager.addComponent<CharacterStateComponent>(controlledEntity);
		NetworkIdComponent* networkId = worldEntityManager.addComponent<NetworkIdComponent>(controlledEntity);
		networkId->setId(0);
	}
	{
		ClientGameDataComponent* clientGameData = getWorldHolder().getWorld().getWorldComponents().getOrAddComponent<ClientGameDataComponent>();
		clientGameData->setControlledPlayer(controlledEntity);
		NetworkIdMappingComponent* networkIdMapping = getWorldHolder().getWorld().getWorldComponents().getOrAddComponent<NetworkIdMappingComponent>();
		networkIdMapping->getNetworkIdToEntityRef().emplace(0, controlledEntity);
	}
	{
		ConnectionManagerComponent* connectionManager = getWorldHolder().getGameData().getGameComponents().getOrAddComponent<ConnectionManagerComponent>();
		connectionManager->setManagerPtr(&mConnectionManager);
	}

	Game::preStart(arguments);
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

	processMoveCorrections();
}

void TankClientGame::fixedTimeUpdate(float dt)
{
	SCOPED_PROFILER("TankClientGame::fixedTimeUpdate");

	getWorldHolder().getWorld().addNewFrameToTheHistory();

	Game::fixedTimeUpdate(dt);

	const auto [time] = getWorldHolder().getWorld().getWorldComponents().getComponents<const TimeComponent>();
	saveMovesForLastFrame(time->getValue().lastFixedUpdateIndex, time->getValue().lastFixedUpdateTimestamp);
}

void TankClientGame::dynamicTimePostFrameUpdate(float dt, int processedFixedUpdates)
{
	SCOPED_PROFILER("TankClientGame::dynamicTimePostFrameUpdate");

	Game::dynamicTimePostFrameUpdate(dt, processedFixedUpdates);
	if (mShouldQuitGameNextTick)
	{
		mShouldQuitGame = true;
	}
}

void TankClientGame::initSystems()
{
	SCOPED_PROFILER("TankClientGame::initSystems");

	AssertFatal(getEngine(), "TankClientGame created without Engine. We're going to crash");

	getPreFrameSystemsManager().registerSystem<InputSystem>(getWorldHolder(), getInputData());
	getPreFrameSystemsManager().registerSystem<ClientInputSendSystem>(getWorldHolder());
	getPreFrameSystemsManager().registerSystem<ClientNetworkSystem>(getWorldHolder(), mShouldQuitGameNextTick);
	getGameLogicSystemsManager().registerSystem<ControlSystem>(getWorldHolder());
	getGameLogicSystemsManager().registerSystem<DeadEntitiesDestructionSystem>(getWorldHolder());
	getGameLogicSystemsManager().registerSystem<MovementSystem>(getWorldHolder());
	getGameLogicSystemsManager().registerSystem<CharacterStateSystem>(getWorldHolder());
	getGameLogicSystemsManager().registerSystem<AnimationSystem>(getWorldHolder());
	getPostFrameSystemsManager().registerSystem<ResourceStreamingSystem>(getWorldHolder(), getResourceManager());
	getPostFrameSystemsManager().registerSystem<RenderSystem>(getWorldHolder(), getResourceManager(), getThreadPool());
	getPostFrameSystemsManager().registerSystem<DebugDrawSystem>(getWorldHolder(), getResourceManager());

#ifdef IMGUI_ENABLED
	getPostFrameSystemsManager().registerSystem<ImguiSystem>(mImguiDebugData, *getEngine());
#endif // IMGUI_ENABLED
}

void TankClientGame::correctUpdates(u32 lastUpdateIdxWithAuthoritativeMoves)
{
	SCOPED_PROFILER("TankClientGame::correctUpdates");

	World& world = getWorldHolder().getWorld();

	// don't store references to component data, since the components will be destroyed during history rewinding
	const TimeData lastUpdateTime = std::get<0>(world.getWorldComponents().getComponents<const TimeComponent>())->getValue();
	const float fixedUpdateDt = lastUpdateTime.lastFixedUpdateDt;

	AssertFatal(lastUpdateIdxWithAuthoritativeMoves <= lastUpdateTime.lastFixedUpdateIndex, "We can't correct updates from the future");
	const u32 framesToResimulate = lastUpdateTime.lastFixedUpdateIndex - lastUpdateIdxWithAuthoritativeMoves;

	auto [clientMovesHistory] = world.getNotRewindableWorldComponents().getComponents<ClientMovesHistoryComponent>();
	MovementHistory& movesHistory = clientMovesHistory->getDataRef();
	std::vector<MovementUpdateData>& updates = movesHistory.updates;

	// unwind the history back
	world.unwindBackInHistory(framesToResimulate);
	updates.erase(updates.begin() + (updates.size() - framesToResimulate), updates.end());
	movesHistory.lastUpdateIdx -= framesToResimulate;

	// apply moves to the diverged frame
	std::unordered_map<Entity, EntityMoveData> entityMoves;
	for (const EntityMoveData& move : updates.back().moves)
	{
		entityMoves.emplace(move.entity, move);
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
	const std::vector<GameplayInput::FrameState>& inputs = inputHistory->getInputs();
	AssertFatal(inputs.size() >= framesToResimulate, "Size of input history can't be less than size of move history");
	size_t inputHistoryIndexShift = inputs.size() - framesToResimulate;
	for (u32 i = 0; i < framesToResimulate; ++i)
	{
		ClientGameDataComponent* clientGameData = getWorldHolder().getWorld().getWorldComponents().getOrAddComponent<ClientGameDataComponent>();
		OptionalEntity optionalControlledEntity = clientGameData->getControlledPlayer();

		if (!optionalControlledEntity.isValid())
		{
			ReportError("The controlled entity should be set when we do movement correction on the client");
			break;
		}

		EntityManager& frameEntityManager = getWorldHolder().getWorld().getEntityManager();

		auto [gameplayInput] = frameEntityManager.getEntityComponents<GameplayInputComponent>(optionalControlledEntity.getEntity());
		if (gameplayInput == nullptr)
		{
			gameplayInput = frameEntityManager.addComponent<GameplayInputComponent>(optionalControlledEntity.getEntity());
		}

		gameplayInput->setCurrentFrameState(inputs[i + inputHistoryIndexShift]);

		fixedTimeUpdate(fixedUpdateDt);
	}
}

void TankClientGame::processMoveCorrections()
{
	SCOPED_PROFILER("TankGameClient::processMoveCorrections");

	constexpr size_t MAX_STORED_UPDATES_COUNT = 60;

	World& world = getWorldHolder().getWorld();

	ClientMovesHistoryComponent* clientMovesHistory = world.getNotRewindableWorldComponents().getOrAddComponent<ClientMovesHistoryComponent>();
	std::vector<MovementUpdateData>& updates = clientMovesHistory->getDataRef().updates;

	{
		const u32 lastUpdateIdx = clientMovesHistory->getData().lastUpdateIdx;
		const u32 firstUpdateIdx = static_cast<u32>(lastUpdateIdx + 1 - updates.size());
		const u32 lastConfirmedUpdateIdx = clientMovesHistory->getData().lastConfirmedUpdateIdx;
		const u32 desynchedUpdateIdx = clientMovesHistory->getData().desynchedUpdateIdx;

		AssertFatal(lastConfirmedUpdateIdx != desynchedUpdateIdx, "We can't have the same frame to be confermed and desynched at the same time");

		// if we need to process corrections
		if (desynchedUpdateIdx > lastConfirmedUpdateIdx && desynchedUpdateIdx >= firstUpdateIdx && desynchedUpdateIdx != std::numeric_limits<u32>::max())
		{
			correctUpdates(desynchedUpdateIdx);

			// mark the desynched update as confirmed since now we applied moves fror server to it
			clientMovesHistory->getDataRef().lastConfirmedUpdateIdx = desynchedUpdateIdx;
			clientMovesHistory->getDataRef().desynchedUpdateIdx = std::numeric_limits<u32>::max();
		}
	}

	const u32 lastUpdateIdx = clientMovesHistory->getData().lastUpdateIdx;
	const u32 firstUpdateIdx = static_cast<u32>(lastUpdateIdx + 1 - updates.size());
	const u32 lastConfirmedUpdateIdx = clientMovesHistory->getData().lastConfirmedUpdateIdx;

	// trim confirmed, corrected, or very old frame records
	const size_t updatesCountBeforeTrim = updates.size();
	size_t updatesCountAfterTrim = std::min(updatesCountBeforeTrim, MAX_STORED_UPDATES_COUNT);
	if (lastConfirmedUpdateIdx >= firstUpdateIdx)
	{
		const size_t lastConfirmedUpdateRecordIdx = lastConfirmedUpdateIdx - firstUpdateIdx;
		updatesCountAfterTrim = std::min(updatesCountAfterTrim, updatesCountBeforeTrim - lastConfirmedUpdateRecordIdx - 1);
	}
	const size_t updatesCountToTrim = updatesCountBeforeTrim - updatesCountAfterTrim;

	updates.erase(updates.begin(), updates.begin() + updatesCountToTrim);

	world.trimOldFrames(updatesCountAfterTrim);
}

void TankClientGame::saveMovesForLastFrame(u32 inputUpdateIndex, const GameplayTimestamp& inputUpdateTimestamp)
{
	SCOPED_PROFILER("TankClientGame::saveMovesForLastFrame");
	EntityManager& entityManager = getWorldHolder().getWorld().getEntityManager();
	ClientMovesHistoryComponent* clientMovesHistory = getWorldHolder().getWorld().getNotRewindableWorldComponents().getOrAddComponent<ClientMovesHistoryComponent>();

	std::vector<MovementUpdateData>& updates = clientMovesHistory->getDataRef().updates;
	AssertFatal(inputUpdateIndex == clientMovesHistory->getData().lastUpdateIdx + 1, "We skipped some frames in the movement history. %u %u", inputUpdateIndex, clientMovesHistory->getData().lastUpdateIdx);
	const size_t nextUpdateIndex = updates.size() + inputUpdateIndex - clientMovesHistory->getData().lastUpdateIdx - 1;
	Assert(nextUpdateIndex == updates.size(), "Possibly miscalculated size of the vector. %u %u", nextUpdateIndex, updates.size());
	updates.resize(nextUpdateIndex + 1);
	MovementUpdateData& newUpdateData = updates[nextUpdateIndex];

	entityManager.forEachComponentSetWithEntity<const MovementComponent, const TransformComponent>(
		[&newUpdateData, inputUpdateTimestamp](Entity entity, const MovementComponent* movement, const TransformComponent* transform)
	{
		// only if we moved within some agreed (between client and server) period of time
		const GameplayTimestamp updateTimestamp = movement->getUpdateTimestamp();
		if (updateTimestamp.isInitialized() && updateTimestamp.getIncreasedByUpdateCount(15) > inputUpdateTimestamp)
		{
			// test code, need to use addHash
			newUpdateData.addMove(entity, transform->getLocation(), updateTimestamp);
			// uncomment when stopped testing
			//newUpdateData.addHash(entity, transform->getLocation());
		}
	});

	std::sort(newUpdateData.updateHash.begin(), newUpdateData.updateHash.end());

	clientMovesHistory->getDataRef().lastUpdateIdx = inputUpdateIndex;
}
