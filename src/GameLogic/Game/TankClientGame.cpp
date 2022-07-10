#include "Base/precomp.h"

#include "GameLogic/Game/TankClientGame.h"

#include "Base/Types/TemplateHelpers.h"

#include "GameData/ComponentRegistration/ComponentFactoryRegistration.h"
#include "GameData/ComponentRegistration/ComponentJsonSerializerRegistration.h"

#include "GameData/ComponentRegistration/ComponentFactoryRegistration.h"
#include "GameData/ComponentRegistration/ComponentJsonSerializerRegistration.h"

#include "GameData/Components/CharacterStateComponent.generated.h"
#include "GameData/Components/ClientGameDataComponent.generated.h"
#include "GameData/Components/ClientMovesHistoryComponent.generated.h"
#include "GameData/Components/ConnectionManagerComponent.generated.h"
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

	const auto [time] = getWorldHolder().getWorld().getWorldComponents().getComponents<const TimeComponent>();
	saveMovesForLastFrame(time->getValue().lastFixedUpdateIndex + 1);

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

void TankClientGame::processMoveCorrections()
{
	SCOPED_PROFILER("TankGameClient::processMoveCorrections");

	constexpr size_t MAX_STORED_UPDATES_COUNT = 60;

	World& world = getWorldHolder().getWorld();

	ClientMovesHistoryComponent* clientMovesHistory = world.getNotRewindableWorldComponents().getOrAddComponent<ClientMovesHistoryComponent>();
	std::vector<MovementUpdateData>& updates = clientMovesHistory->getDataRef().updates;
	const u32 lastUpdateIdx = clientMovesHistory->getData().lastUpdateIdx;
	const u32 firstUpdateIdx = lastUpdateIdx + 1 - updates.size();
	const u32 lastConfirmedUpdateIdx = clientMovesHistory->getData().lastConfirmedUpdateIdx;
	const u32 desynchedUpdateIdx = clientMovesHistory->getData().desynchedUpdateIdx;

	AssertFatal(lastConfirmedUpdateIdx != desynchedUpdateIdx, "We can't have the same frame to be confermed and desynched at the same time");

	// if we need to process corrections
	if (desynchedUpdateIdx > lastConfirmedUpdateIdx && desynchedUpdateIdx >= firstUpdateIdx && desynchedUpdateIdx != std::numeric_limits<u32>::max())
	{
		const auto [time] = getWorldHolder().getWorld().getWorldComponents().getComponents<const TimeComponent>();
		LogInfo("Need to process moves correction from updates %d to %d", desynchedUpdateIdx, time->getValue().lastFixedUpdateIndex);
	}
	else
	{
		// trim confirmed or very old frame records
		const size_t updatesCountBeforeTrim = updates.size();
		size_t updatesCountAfterTrim = std::min(updatesCountBeforeTrim, MAX_STORED_UPDATES_COUNT);
		if (lastConfirmedUpdateIdx >= firstUpdateIdx)
		{
			const size_t lastConfirmedUpdateRecordIdx = lastConfirmedUpdateIdx - firstUpdateIdx;
			updatesCountAfterTrim = std::max(updatesCountAfterTrim, updatesCountBeforeTrim - lastConfirmedUpdateRecordIdx - 1);
		}
		const size_t updatesCountToTrim = updatesCountBeforeTrim - updatesCountAfterTrim;

		updates.erase(updates.begin(), updates.begin() + updatesCountToTrim);

		//world.trimOldFrames(updatesAfterTrim);
	}
}

void TankClientGame::saveMovesForLastFrame(u32 inputUpdateIndex)
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
		[&newUpdateData](Entity entity, const MovementComponent* movement, const TransformComponent* transform)
	{
		if (movement->getNextStep() == ZERO_VECTOR && movement->getPreviousStep() == ZERO_VECTOR)
		{
			const Vector2D location = transform->getLocation();
			const Vector2D nextStep = movement->getNextStep();
			newUpdateData.moves.emplace_back(entity, IntVector2D(static_cast<s32>(location.x), static_cast<s32>(location.y)), IntVector2D(static_cast<s32>(nextStep.x), static_cast<s32>(nextStep.y)));
		}
	});

	std::sort(
		newUpdateData.moves.begin(),
		newUpdateData.moves.end(),
		[](const EntityMoveData& l, const EntityMoveData& r)
		{
			return l.entity < r.entity;
		}
	);
	clientMovesHistory->getDataRef().lastUpdateIdx = inputUpdateIndex;
}
