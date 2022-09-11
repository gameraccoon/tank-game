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
#include "GameLogic/Systems/CharacterStateSystem.h"
#include "GameLogic/Systems/ControlSystem.h"
#include "GameLogic/Systems/DeadEntitiesDestructionSystem.h"
#include "GameLogic/Systems/MovementSystem.h"
#include "GameLogic/Systems/RenderSystem.h"
#include "GameLogic/Systems/ResourceStreamingSystem.h"
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

	const bool shouldRender = renderAccessor.has_value();

	initSystems(shouldRender);

	GameDataLoader::LoadWorld(getWorldHolder().getWorld(), arguments.getArgumentValue("world", "test"), getComponentSerializers());
	GameDataLoader::LoadGameData(getGameData(), arguments.getArgumentValue("gameData", "gameData"), getComponentSerializers());

	// if we do debug rendering of server state
	if (shouldRender)
	{
		RenderAccessorComponent* renderAccessorComponent = getGameData().getGameComponents().getOrAddComponent<RenderAccessorComponent>();
		renderAccessorComponent->setAccessor(renderAccessor);
		getWorldHolder().getWorld().getWorldComponents().getOrAddComponent<WorldCachedDataComponent>()->setDrawShift(Vector2D(300.0f, 0.0f));
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

	mConnectionManager.processNetworkEvents();

	processInputCorrections();
}

void TankServerGame::fixedTimeUpdate(float dt)
{
	SCOPED_PROFILER("TankServerGame::fixedTimeUpdate");

	getWorldHolder().getWorld().addNewFrameToTheHistory();

	const auto [time] = getWorldHolder().getWorld().getWorldComponents().getComponents<const TimeComponent>();
	applyInputForCurrentUpdate(time->getValue().lastFixedUpdateIndex + 1);

	Game::fixedTimeUpdate(dt);
}

void TankServerGame::dynamicTimePostFrameUpdate(float dt, int processedFixedTimeUpdates)
{
	SCOPED_PROFILER("TankServerGame::dynamicTimePostFrameUpdate");

	Game::dynamicTimePostFrameUpdate(dt, processedFixedTimeUpdates);

	mConnectionManager.flushMesssagesForAllClientConnections(14436);
}

void TankServerGame::initSystems([[maybe_unused]] bool shouldRender)
{
	SCOPED_PROFILER("TankServerGame::initSystems");

	getPreFrameSystemsManager().registerSystem<ServerNetworkSystem>(getWorldHolder(), mServerPort, mShouldQuitGame);
	getGameLogicSystemsManager().registerSystem<ControlSystem>(getWorldHolder());
	getGameLogicSystemsManager().registerSystem<DeadEntitiesDestructionSystem>(getWorldHolder());
	getGameLogicSystemsManager().registerSystem<MovementSystem>(getWorldHolder());
	getGameLogicSystemsManager().registerSystem<CharacterStateSystem>(getWorldHolder());
	getGameLogicSystemsManager().registerSystem<AnimationSystem>(getWorldHolder());
	getPostFrameSystemsManager().registerSystem<ServerMovesSendSystem>(getWorldHolder());
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
	const TimeData& timeValue = time->getValue();
	const float fixedUpdateDt = timeValue.lastFixedUpdateDt;

	AssertFatal(firstIncorrectUpdateIdx <= timeValue.lastFixedUpdateIndex, "We can't correct updates from the future");
	const u32 framesToResimulate = timeValue.lastFixedUpdateIndex - firstIncorrectUpdateIdx + 1;

	LogInfo("Correct server updates from %u to %u", firstIncorrectUpdateIdx, timeValue.lastFixedUpdateIndex);

	getWorldHolder().getWorld().unwindBackInHistory(framesToResimulate);

	for (u32 i = 0; i < framesToResimulate; ++i)
	{
		fixedTimeUpdate(fixedUpdateDt);
	}
}

void TankServerGame::processInputCorrections()
{
	SCOPED_PROFILER("TankServerGame::processInputCorrections");

	const auto [time] = getWorldHolder().getWorld().getWorldComponents().getComponents<const TimeComponent>();

	// first real update has index 1
	if (time->getValue().lastFixedUpdateIndex < 1)
	{
		return;
	}

	ServerConnectionsComponent* serverConnections = getWorldHolder().getWorld().getNotRewindableWorldComponents().getOrAddComponent<ServerConnectionsComponent>();
	const u32 lastProcessedUpdateIdx = time->getValue().lastFixedUpdateIndex;
	// first update of the input that diverged from with prediction for at least one client
	u32 firstUpdateToCorrect = lastProcessedUpdateIdx + 1;
	// index of first frame when we don't have input from some client yet
	u32 firstNotConfirmedUpdateIdx = lastProcessedUpdateIdx + 1;

	constexpr u32 MAX_STORED_UPDATES_COUNT = 60;

	for (auto& [_, inputHistory] : serverConnections->getInputsRef())
	{
		if (inputHistory.inputs.empty())
		{
			continue;
		}

		Assert(inputHistory.indexShift != std::numeric_limits<s32>::max(), "If we have input, we should have indexShift filled");

		const u32 lastAbsoluteInputUpdateIdx = inputHistory.lastInputUpdateIdx + inputHistory.indexShift;
		firstNotConfirmedUpdateIdx = std::min(firstNotConfirmedUpdateIdx, lastAbsoluteInputUpdateIdx);
		// will wrap after ~8.5 years of gameplay
		const u32 inputFromFutureCount = ((lastAbsoluteInputUpdateIdx > lastProcessedUpdateIdx) ? lastAbsoluteInputUpdateIdx - lastProcessedUpdateIdx : 0);
		const size_t iEnd = std::min(inputHistory.inputs.size(), inputHistory.inputs.size() - static_cast<size_t>(inputFromFutureCount));
		for (size_t i = 1; i < iEnd; ++i)
		{
			if (inputHistory.inputs[i] != inputHistory.inputs[i - 1])
			{
				const u32 firstIncorrectUpdate = static_cast<u32>(lastAbsoluteInputUpdateIdx + 1 + i - inputHistory.inputs.size());
				firstUpdateToCorrect = std::min(firstUpdateToCorrect, firstIncorrectUpdate);
				break;
			}
		}
	}

	// don't try to correct frames that we have already trimmed
	firstUpdateToCorrect = std::max(firstUpdateToCorrect, static_cast<u32>(lastProcessedUpdateIdx + 2 - getWorldHolder().getWorld().getStoredFramesCount()));

	// limit amount of frames to a predefined value
	firstNotConfirmedUpdateIdx = std::max(firstNotConfirmedUpdateIdx + MAX_STORED_UPDATES_COUNT, lastProcessedUpdateIdx) - MAX_STORED_UPDATES_COUNT;

	if (firstUpdateToCorrect <= lastProcessedUpdateIdx)
	{
		correctUpdates(firstUpdateToCorrect);
	}

	for (auto& [_, inputHistory] : serverConnections->getInputsRef())
	{
		AssertFatal(inputHistory.lastInputUpdateIdx + 1 >= inputHistory.inputs.size(), "We can't have input stored for frames with negative index");
		const u32 firstIdxFrame = static_cast<u32>(inputHistory.lastInputUpdateIdx + 1 - inputHistory.inputs.size());
		const size_t firstIdxToKeep = std::max(firstIdxFrame, firstNotConfirmedUpdateIdx - inputHistory.indexShift) - firstIdxFrame;

		if (!inputHistory.inputs.empty() && firstIdxToKeep > 0)
		{
			inputHistory.inputs.erase(inputHistory.inputs.begin(), inputHistory.inputs.begin() + std::min(firstIdxToKeep, inputHistory.inputs.size() - 1));
		}
	}

	AssertFatal(lastProcessedUpdateIdx + 1 >= firstNotConfirmedUpdateIdx, "firstNotConfirmedFrameIdx can't be greater than lastProcessedUpdateIdx");
	getWorldHolder().getWorld().trimOldFrames(lastProcessedUpdateIdx + 1 - firstNotConfirmedUpdateIdx);
}

void TankServerGame::applyInputForCurrentUpdate(u32 inputUpdateIndex)
{
	SCOPED_PROFILER("TankServerGame::updateInputForLastFrame");
	EntityManager& entityManager = getWorldHolder().getWorld().getEntityManager();
	ServerConnectionsComponent* serverConnections = getWorldHolder().getWorld().getNotRewindableWorldComponents().getOrAddComponent<ServerConnectionsComponent>();
	for (auto [connectionId, optionalEntity] : serverConnections->getControlledPlayers())
	{
		if (optionalEntity.isValid())
		{
			auto [gameplayInput] = entityManager.getEntityComponents<GameplayInputComponent>(optionalEntity.getEntity());
			if (gameplayInput == nullptr)
			{
				gameplayInput = entityManager.addComponent<GameplayInputComponent>(optionalEntity.getEntity());
			}
			const Input::InputHistory& inputHistory = serverConnections->getInputsRef()[connectionId];
			if (!inputHistory.inputs.empty())
			{
				const u32 absoluteLastUpdateIdx = inputHistory.lastInputUpdateIdx + inputHistory.indexShift;
				if (inputUpdateIndex > absoluteLastUpdateIdx)
				{
					inputUpdateIndex = absoluteLastUpdateIdx;
				}

				if (inputUpdateIndex + inputHistory.inputs.size() - inputHistory.indexShift > inputHistory.lastInputUpdateIdx)
				{
					size_t index = inputUpdateIndex - inputHistory.indexShift - (inputHistory.lastInputUpdateIdx + 1 - inputHistory.inputs.size());
					gameplayInput->setCurrentFrameState(inputHistory.inputs[index]);
				}
			}
		}
	}
}
