#include "Base/precomp.h"

#include "GameLogic/Game/TankClientGame.h"

#include "Base/Types/TemplateHelpers.h"

#include "GameData/ComponentRegistration/ComponentFactoryRegistration.h"
#include "GameData/ComponentRegistration/ComponentJsonSerializerRegistration.h"

#include "GameData/ComponentRegistration/ComponentFactoryRegistration.h"
#include "GameData/ComponentRegistration/ComponentJsonSerializerRegistration.h"

#include "GameData/Components/RenderAccessorComponent.generated.h"
#include "GameData/Components/SpriteCreatorComponent.generated.h"
#include "GameData/Components/TransformComponent.generated.h"
#include "GameData/Components/MovementComponent.generated.h"
#include "GameData/Components/InputHistoryComponent.generated.h"
#include "GameData/Components/ClientGameDataComponent.generated.h"
#include "GameData/Components/CharacterStateComponent.generated.h"
#include "GameData/Components/NetworkIdComponent.generated.h"
#include "GameData/Components/NetworkIdMappingComponent.generated.h"
#include "GameData/Components/ConnectionManagerComponent.generated.h"

#include "Utils/Application/ArgumentsParser.h"
#include "Utils/World/GameDataLoader.h"

#include "HAL/Base/Engine.h"

#include "GameLogic/Systems/AnimationSystem.h"
#include "GameLogic/Systems/CharacterStateSystem.h"
#include "GameLogic/Systems/ClientInpuntSendSystem.h"
#include "GameLogic/Systems/ClientNetworkSystem.h"
#include "GameLogic/Systems/ControlSystem.h"
#include "GameLogic/Systems/DeadEntitiesDestructionSystem.h"
#include "GameLogic/Systems/DebugDrawSystem.h"
#include "GameLogic/Systems/MovementSystem.h"
#include "GameLogic/Systems/RenderSystem.h"
#include "GameLogic/Systems/ResourceStreamingSystem.h"

#ifdef IMGUI_ENABLED
#include "GameLogic/Systems/ImguiSystem.h"
#endif // IMGUI_ENABLED

#ifdef ENABLE_SCOPED_PROFILER
#include "Utils/Profiling/ProfileDataWriter.h"
#endif // ENABLE_SCOPED_PROFILER

#include "GameLogic/Initialization/StateMachines.h"

void TankClientGame::preStart(ArgumentsParser& arguments, RenderAccessor& renderAccessor)
{
	SCOPED_PROFILER("TankClientGame::preStart");

	ComponentsRegistration::RegisterComponents(getComponentFactory());
	ComponentsRegistration::RegisterJsonSerializers(getComponentSerializers());

	initSystems();

	GameDataLoader::LoadWorld(getWorldHolder().getWorld(), arguments.getArgumentValue("world", "test"), getComponentSerializers());
	GameDataLoader::LoadGameData(getGameData(), arguments.getArgumentValue("gameData", "gameData"), getComponentSerializers());

	RenderAccessorComponent* renderAccessorComponent = getGameData().getGameComponents().getOrAddComponent<RenderAccessorComponent>();
	renderAccessorComponent->setAccessor(&renderAccessor);

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

void TankClientGame::initSystems()
{
	SCOPED_PROFILER("TankClientGame::initSystems");

	AssertFatal(getEngine(), "TankClientGame created without Engine. We're going to crash");

	getPreFrameSystemsManager().registerSystem<ControlSystem>(getWorldHolder(), getInputData());
	getGameLogicSystemsManager().registerSystem<ClientInputSendSystem>(getWorldHolder());
	getGameLogicSystemsManager().registerSystem<DeadEntitiesDestructionSystem>(getWorldHolder());
	getGameLogicSystemsManager().registerSystem<MovementSystem>(getWorldHolder(), getTime());
	getGameLogicSystemsManager().registerSystem<CharacterStateSystem>(getWorldHolder(), getTime());
	getGameLogicSystemsManager().registerSystem<AnimationSystem>(getWorldHolder(), getTime());
	getPostFrameSystemsManager().registerSystem<ResourceStreamingSystem>(getWorldHolder(), getResourceManager());
	getPostFrameSystemsManager().registerSystem<RenderSystem>(getWorldHolder(), getTime(), getResourceManager(), getThreadPool());
	getPostFrameSystemsManager().registerSystem<DebugDrawSystem>(getWorldHolder(), getTime(), getResourceManager());
	getPostFrameSystemsManager().registerSystem<ClientNetworkSystem>(getWorldHolder(), mShouldQuitGameNextTick);

#ifdef IMGUI_ENABLED
	getPostFrameSystemsManager().registerSystem<ImguiSystem>(mImguiDebugData, *getEngine());
#endif // IMGUI_ENABLED
}

void TankClientGame::initResources()
{
	SCOPED_PROFILER("TankGameClient::initResources");
	getResourceManager().loadAtlasesData("resources/atlas/atlas-list.json");
	Game::initResources();
}

void TankClientGame::dynamicTimePostFrameUpdate(float dt)
{
	Game::dynamicTimePostFrameUpdate(dt);
	if (mShouldQuitGameNextTick)
	{
		mShouldQuitGame = true;
	}
}
