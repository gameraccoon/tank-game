#include "Base/precomp.h"

#include "GameLogic/Game/TankServerGame.h"

#include "Base/Types/TemplateHelpers.h"

#include "GameData/ComponentRegistration/ComponentFactoryRegistration.h"
#include "GameData/ComponentRegistration/ComponentJsonSerializerRegistration.h"

#include "GameData/Components/CharacterStateComponent.generated.h"
#include "GameData/Components/ConnectionManagerComponent.generated.h"
#include "GameData/Components/InputHistoryComponent.generated.h"
#include "GameData/Components/MovementComponent.generated.h"
#include "GameData/Components/NetworkIdComponent.generated.h"
#include "GameData/Components/NetworkIdMappingComponent.generated.h"
#include "GameData/Components/RenderAccessorComponent.generated.h"
#include "GameData/Components/ServerConnectionsComponent.generated.h"
#include "GameData/Components/TransformComponent.generated.h"
#include "GameData/Components/WorldCachedDataComponent.generated.h"

#include "HAL/Base/Engine.h"

#include "Utils/Application/ArgumentsParser.h"
#include "Utils/World/GameDataLoader.h"

#include "GameLogic/Initialization/StateMachines.h"
#include "GameLogic/Render/RenderAccessor.h"
#include "GameLogic/Systems/AnimationSystem.h"
#include "GameLogic/Systems/CharacterStateSystem.h"
#include "GameLogic/Systems/ControlSystem.h"
#include "GameLogic/Systems/DeadEntitiesDestructionSystem.h"
#include "GameLogic/Systems/MovementSystem.h"
#include "GameLogic/Systems/RenderSystem.h"
#include "GameLogic/Systems/ResourceStreamingSystem.h"
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

	initSystems(renderAccessor.has_value());

	GameDataLoader::LoadWorld(getWorldHolder().getWorld(), arguments.getArgumentValue("world", "test"), getComponentSerializers());
	GameDataLoader::LoadGameData(getGameData(), arguments.getArgumentValue("gameData", "gameData"), getComponentSerializers());

	// if we do debug rendering of server state
	if (renderAccessor.has_value())
	{
		RenderAccessorComponent* renderAccessorComponent = getGameData().getGameComponents().getOrAddComponent<RenderAccessorComponent>();
		renderAccessorComponent->setAccessor(renderAccessor);
		getWorldHolder().getWorld().getWorldComponents().getOrAddComponent<WorldCachedDataComponent>()->setDrawShift(Vector2D(300.0f, 0.0f));
	}

	EntityManager& worldEntityManager = getWorldHolder().getWorld().getEntityManager();
	Entity controlledEntity = worldEntityManager.addEntity();
	{
		TransformComponent* transform = worldEntityManager.addComponent<TransformComponent>(controlledEntity);
		transform->setLocation(Vector2D(50, 50));
		MovementComponent* movement = worldEntityManager.addComponent<MovementComponent>(controlledEntity);
		movement->setOriginalSpeed(20.0f);
		worldEntityManager.addComponent<CharacterStateComponent>(controlledEntity);
		NetworkIdComponent* networkId = worldEntityManager.addComponent<NetworkIdComponent>(controlledEntity);
		networkId->setId(0);
	}
	{
		NetworkIdMappingComponent* networkIdMapping = getWorldHolder().getWorld().getWorldComponents().getOrAddComponent<NetworkIdMappingComponent>();
		networkIdMapping->getNetworkIdToEntityRef().emplace(0, controlledEntity);
	}
	{
		ConnectionManagerComponent* connectionManager = getWorldHolder().getGameData().getGameComponents().getOrAddComponent<ConnectionManagerComponent>();
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

	processInputCorrections();
}

void TankServerGame::dynamicTimePostFrameUpdate(float dt, int processedFixedTimeUpdates)
{
	SCOPED_PROFILER("TankServerGame::dynamicTimePostFrameUpdate");
	Game::dynamicTimePostFrameUpdate(dt, processedFixedTimeUpdates);
	// copy current world into the history vector
	getWorldHolder().getWorld().addNewFrameToTheHistory();
}

void TankServerGame::initSystems(bool shouldRender)
{
	SCOPED_PROFILER("TankServerGame::initSystems");

	getPreFrameSystemsManager().registerSystem<ServerNetworkSystem>(getWorldHolder(), mShouldQuitGame);
	getGameLogicSystemsManager().registerSystem<ControlSystem>(getWorldHolder());
	getGameLogicSystemsManager().registerSystem<DeadEntitiesDestructionSystem>(getWorldHolder());
	getGameLogicSystemsManager().registerSystem<MovementSystem>(getWorldHolder(), getTime());
	getGameLogicSystemsManager().registerSystem<CharacterStateSystem>(getWorldHolder(), getTime());
	getGameLogicSystemsManager().registerSystem<AnimationSystem>(getWorldHolder(), getTime());
	getPostFrameSystemsManager().registerSystem<ResourceStreamingSystem>(getWorldHolder(), getResourceManager());

	if (shouldRender)
	{
		getPostFrameSystemsManager().registerSystem<RenderSystem>(getWorldHolder(), getTime(), getResourceManager(), getThreadPool());
	}
}

void TankServerGame::processInputCorrections()
{
	SCOPED_PROFILER("TankServerGame::processInputCorrections");

	ServerConnectionsComponent* serverConnections = getWorldHolder().getWorld().getNotRewindableWorldComponents().getOrAddComponent<ServerConnectionsComponent>();
	const u32 lastProcessedUpdateIdx = getTime().lastFixedUpdateIndex;
	// first update input for that diverged from with prediction for at least one client
	u32 firstUpdateToRewind = lastProcessedUpdateIdx + 1;
	// index of first frame when we don't have input from some client yet
	u32 firstNotConfirmedUpdateIdx = lastProcessedUpdateIdx;

	constexpr u32 MAX_STORED_UPDATES_COUNT = 60;
	constexpr u32 MIN_STORED_UPDATES_COUNT = 0;

	for (auto& [_, inputHistory] : serverConnections->getInputsRef())
	{
		const u32 lastInputUpdateIdx = inputHistory.lastInputUpdateIdx;
		firstNotConfirmedUpdateIdx = std::min(firstNotConfirmedUpdateIdx, lastInputUpdateIdx);
		// will wrap after ~8.5 years of gameplay
		const u32 inputFromFutureCount = ((lastInputUpdateIdx > lastProcessedUpdateIdx) ? lastInputUpdateIdx - lastProcessedUpdateIdx : 0);
		const size_t iEnd = std::min(inputHistory.inputs.size(), inputHistory.inputs.size() - static_cast<size_t>(inputFromFutureCount));
		for (size_t i = 1; i < iEnd; ++i)
		{
			if (inputHistory.inputs[i] != inputHistory.inputs[i - 1])
			{
				firstUpdateToRewind = std::min(firstUpdateToRewind, lastInputUpdateIdx - static_cast<u32>(i));
				break;
			}
		}
	}

	// limit amount of frames to a predefined value
	firstNotConfirmedUpdateIdx = std::max(firstNotConfirmedUpdateIdx + MAX_STORED_UPDATES_COUNT, lastProcessedUpdateIdx) - MAX_STORED_UPDATES_COUNT;

	// ToDo: correction logic goes here
	if (firstUpdateToRewind < lastProcessedUpdateIdx + 1)
	{
		//LogInfo("%d last frames should be corrected", lastProcessedUpdateIdx + 1 - firstUpdateToRewind);
	}

	for (auto& [_, inputHistory] : serverConnections->getInputsRef())
	{
		AssertFatal(inputHistory.lastInputUpdateIdx + 1 >= inputHistory.inputs.size(), "We can't have input stored for frames with negative index");
		const u32 firstIdxFrame = inputHistory.lastInputUpdateIdx + 1 - inputHistory.inputs.size();
		const size_t firstIdxToKeep = std::max(firstIdxFrame, firstNotConfirmedUpdateIdx) - firstIdxFrame;

		if (!inputHistory.inputs.empty() && firstIdxToKeep > 0)
		{
			inputHistory.inputs.erase(inputHistory.inputs.begin(), inputHistory.inputs.begin() + std::min(firstIdxToKeep, inputHistory.inputs.size() - MIN_STORED_UPDATES_COUNT));
		}
	}

	AssertFatal(lastProcessedUpdateIdx >= firstNotConfirmedUpdateIdx, "firstNotConfirmedFrameIdx can't be greater than lastProcessedUpdateIdx");
	getWorldHolder().getWorld().trimOldFrames(std::max(MIN_STORED_UPDATES_COUNT, lastProcessedUpdateIdx + MIN_STORED_UPDATES_COUNT - firstNotConfirmedUpdateIdx));
}
