#include "Base/precomp.h"

#include <gtest/gtest.h>
#include <memory>

#include "GameData/ComponentRegistration/ComponentFactoryRegistration.h"
#include "GameData/Components/TimeComponent.generated.h"
#include "GameData/GameData.h"
#include "GameData/Input/GameplayInput.h"
#include "GameData/World.h"

#include "Utils/Network/GameStateRewinder.h"
#include "Utils/Network/Messages/PlayerInputMessage.h"
#include "Utils/SharedManagers/WorldHolder.h"


namespace PlayerInputMessageInternal
{
	struct TestGame
	{
		ComponentFactory componentFactory;
		RaccoonEcs::IncrementalEntityGenerator entityGenerator;
		World world{componentFactory, entityGenerator};
		GameData gameData{componentFactory};
		WorldHolder worldHolder{nullptr, gameData};
		GameStateRewinder stateRewinder{GameStateRewinder::HistoryType::Client, componentFactory, entityGenerator, worldHolder};
	};

	static std::unique_ptr<TestGame> CreateGameInstance()
	{
		std::unique_ptr<TestGame> testGame = std::make_unique<TestGame>();
		ComponentsRegistration::RegisterComponents(testGame->componentFactory);
		TimeComponent* time = testGame->world.getWorldComponents().addComponent<TimeComponent>();
		time->setValue(&testGame->stateRewinder.getTimeData());
		return testGame;
	}
}

TEST(PlayerInputMessage, SerializeAndDeserializeFirstInput_AllInputAdded)
{
	using namespace PlayerInputMessageInternal;
	auto clientGame = CreateGameInstance();

	{
		GameplayInput::FrameState frameState;
		frameState.updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::Inactive, GameplayTimestamp(0));
		frameState.updateAxis(GameplayInput::InputAxis::MoveHorizontal, 0.0f);
		clientGame->stateRewinder.addFrameToInputHistory(1u, frameState);
		frameState.updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::JustActivated, GameplayTimestamp(1));
		frameState.updateAxis(GameplayInput::InputAxis::MoveHorizontal, 0.5f);
		clientGame->stateRewinder.addFrameToInputHistory(2u, frameState);
		frameState.updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::Active, GameplayTimestamp(2));
		frameState.updateAxis(GameplayInput::InputAxis::MoveHorizontal, 1.0f);
		clientGame->stateRewinder.addFrameToInputHistory(3u, frameState);
		frameState.updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::JustDeactivated, GameplayTimestamp(3));
		frameState.updateAxis(GameplayInput::InputAxis::MoveHorizontal, 0.75f);
		clientGame->stateRewinder.addFrameToInputHistory(4u, frameState);
		frameState.updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::Inactive, GameplayTimestamp(4));
		frameState.updateAxis(GameplayInput::InputAxis::MoveHorizontal, 0.25f);
		clientGame->stateRewinder.addFrameToInputHistory(5u, frameState);
	}

	HAL::ConnectionManager::Message message = Network::CreatePlayerInputMessage(clientGame->stateRewinder);

	{
		auto serverGame = CreateGameInstance();
		const ConnectionId connectionId = 1;
		serverGame->stateRewinder.onClientConnected(connectionId, 0u);
		serverGame->stateRewinder.getTimeData().lastFixedUpdateIndex = 6;
		Network::ApplyPlayerInputMessage(serverGame->world, serverGame->stateRewinder, message, connectionId);

		const Input::InputHistory& inputHistory = serverGame->stateRewinder.getInputHistoryForClient(connectionId);

		ASSERT_EQ(inputHistory.inputs.size(), static_cast<size_t>(5));
		EXPECT_EQ(inputHistory.inputs[0].getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::Inactive);
		EXPECT_EQ(inputHistory.inputs[0].getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(0));
		EXPECT_FLOAT_EQ(inputHistory.inputs[0].getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 0.0f);
		EXPECT_EQ(inputHistory.inputs[1].getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::JustActivated);
		EXPECT_EQ(inputHistory.inputs[1].getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(1));
		EXPECT_FLOAT_EQ(inputHistory.inputs[1].getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 0.5f);
		EXPECT_EQ(inputHistory.inputs[2].getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::Active);
		EXPECT_EQ(inputHistory.inputs[2].getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(1));
		EXPECT_FLOAT_EQ(inputHistory.inputs[2].getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 1.0f);
		EXPECT_EQ(inputHistory.inputs[3].getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::JustDeactivated);
		EXPECT_EQ(inputHistory.inputs[3].getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(3));
		EXPECT_FLOAT_EQ(inputHistory.inputs[3].getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 0.75f);
		EXPECT_EQ(inputHistory.inputs[4].getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::Inactive);
		EXPECT_EQ(inputHistory.inputs[4].getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(3));
		EXPECT_FLOAT_EQ(inputHistory.inputs[4].getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 0.25f);
		EXPECT_EQ(inputHistory.lastInputUpdateIdx, static_cast<u32>(5));
		EXPECT_EQ(inputHistory.indexShift, static_cast<s32>(1));
	}
}

TEST(PlayerInputMessage, SerializeAndDeserializePartlyKnownInput_NewInputsAdded)
{
	using namespace PlayerInputMessageInternal;
	auto clientGame = CreateGameInstance();

	{
		GameplayInput::FrameState frameState;
		frameState.updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::Inactive, GameplayTimestamp(0));
		frameState.updateAxis(GameplayInput::InputAxis::MoveHorizontal, 0.0f);
		clientGame->stateRewinder.addFrameToInputHistory(1u, frameState);
		frameState.updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::JustActivated, GameplayTimestamp(1));
		frameState.updateAxis(GameplayInput::InputAxis::MoveHorizontal, 0.5f);
		clientGame->stateRewinder.addFrameToInputHistory(2u, frameState);
		frameState.updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::Active, GameplayTimestamp(2));
		frameState.updateAxis(GameplayInput::InputAxis::MoveHorizontal, 1.0f);
		clientGame->stateRewinder.addFrameToInputHistory(3u, frameState);
		frameState.updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::JustDeactivated, GameplayTimestamp(3));
		frameState.updateAxis(GameplayInput::InputAxis::MoveHorizontal, 0.75f);
		clientGame->stateRewinder.addFrameToInputHistory(4u, frameState);
		frameState.updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::Inactive, GameplayTimestamp(4));
		frameState.updateAxis(GameplayInput::InputAxis::MoveHorizontal, 0.25f);
		clientGame->stateRewinder.addFrameToInputHistory(5u, frameState);
	}

	HAL::ConnectionManager::Message message = Network::CreatePlayerInputMessage(clientGame->stateRewinder);

	{
		auto serverGame = CreateGameInstance();
		ConnectionId connectionId = 1;
		{
			serverGame->stateRewinder.onClientConnected(connectionId, 0u);

			Input::InputHistory& inputHistory = serverGame->stateRewinder.getInputHistoryForClient(connectionId);
			inputHistory.inputs.resize(3);
			inputHistory.inputs[0].updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::Inactive, GameplayTimestamp(0));
			inputHistory.inputs[0].updateAxis(GameplayInput::InputAxis::MoveHorizontal, 0.0f);
			inputHistory.inputs[1].updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::JustActivated, GameplayTimestamp(1));
			inputHistory.inputs[1].updateAxis(GameplayInput::InputAxis::MoveHorizontal, 0.5f);
			inputHistory.inputs[2].updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::Active, GameplayTimestamp(1));
			inputHistory.inputs[2].updateAxis(GameplayInput::InputAxis::MoveHorizontal, 1.0f);
			inputHistory.lastInputUpdateIdx = 3;
			serverGame->stateRewinder.getTimeData().lastFixedUpdateIndex = 6;
		}

		Network::ApplyPlayerInputMessage(serverGame->world, serverGame->stateRewinder, message, connectionId);

		const Input::InputHistory& inputHistory = serverGame->stateRewinder.getInputHistoryForClient(connectionId);

		ASSERT_EQ(inputHistory.inputs.size(), static_cast<size_t>(5));
		EXPECT_EQ(inputHistory.inputs[0].getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::Inactive);
		EXPECT_EQ(inputHistory.inputs[0].getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(0));
		EXPECT_FLOAT_EQ(inputHistory.inputs[0].getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 0.0f);
		EXPECT_EQ(inputHistory.inputs[1].getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::JustActivated);
		EXPECT_EQ(inputHistory.inputs[1].getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(1));
		EXPECT_FLOAT_EQ(inputHistory.inputs[1].getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 0.5f);
		EXPECT_EQ(inputHistory.inputs[2].getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::Active);
		EXPECT_EQ(inputHistory.inputs[2].getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(1));
		EXPECT_FLOAT_EQ(inputHistory.inputs[2].getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 1.0f);
		EXPECT_EQ(inputHistory.inputs[3].getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::JustDeactivated);
		EXPECT_EQ(inputHistory.inputs[3].getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(3));
		EXPECT_FLOAT_EQ(inputHistory.inputs[3].getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 0.75f);
		EXPECT_EQ(inputHistory.inputs[4].getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::Inactive);
		EXPECT_EQ(inputHistory.inputs[4].getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(3));
		EXPECT_FLOAT_EQ(inputHistory.inputs[4].getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 0.25f);
		EXPECT_EQ(inputHistory.lastInputUpdateIdx, static_cast<u32>(5));
		EXPECT_EQ(inputHistory.indexShift, static_cast<s32>(1));
	}
}

TEST(PlayerInputMessage, SerializeAndDeserializeInputWithAGap_NewInputsAddedMissedInputsPredicted)
{
	using namespace PlayerInputMessageInternal;
	auto clientGame = CreateGameInstance();

	{
		GameplayInput::FrameState frameState;
		frameState.updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::Inactive, GameplayTimestamp(0));
		frameState.updateAxis(GameplayInput::InputAxis::MoveHorizontal, 0.0f);
		clientGame->stateRewinder.addFrameToInputHistory(1u, frameState);
		frameState.updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::JustActivated, GameplayTimestamp(1));
		frameState.updateAxis(GameplayInput::InputAxis::MoveHorizontal, 0.5f);
		clientGame->stateRewinder.addFrameToInputHistory(2u, frameState);
		frameState.updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::Active, GameplayTimestamp(2));
		frameState.updateAxis(GameplayInput::InputAxis::MoveHorizontal, 1.0f);
		clientGame->stateRewinder.addFrameToInputHistory(3u, frameState);
		frameState.updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::Active, GameplayTimestamp(3));
		frameState.updateAxis(GameplayInput::InputAxis::MoveHorizontal, 1.0f);
		clientGame->stateRewinder.addFrameToInputHistory(4u, frameState);
		frameState.updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::Active, GameplayTimestamp(4));
		frameState.updateAxis(GameplayInput::InputAxis::MoveHorizontal, 1.0f);
		clientGame->stateRewinder.addFrameToInputHistory(5u, frameState);
		frameState.updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::JustDeactivated, GameplayTimestamp(5));
		frameState.updateAxis(GameplayInput::InputAxis::MoveHorizontal, 0.75f);
		clientGame->stateRewinder.addFrameToInputHistory(6u, frameState);
		frameState.updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::Inactive, GameplayTimestamp(6));
		frameState.updateAxis(GameplayInput::InputAxis::MoveHorizontal, 0.25f);
		clientGame->stateRewinder.addFrameToInputHistory(7u, frameState);
	}

	// keep only two inputs from above
	clientGame->stateRewinder.clearOldInputs(6u);
	HAL::ConnectionManager::Message message = Network::CreatePlayerInputMessage(clientGame->stateRewinder);

	{
		auto serverGame = CreateGameInstance();
		serverGame->stateRewinder.getTimeData().lastFixedUpdateIndex = 8;
		ConnectionId connectionId = 1;
		{
			serverGame->stateRewinder.onClientConnected(connectionId, 0u);

			Input::InputHistory& inputHistory = serverGame->stateRewinder.getInputHistoryForClient(connectionId);

			inputHistory.inputs.resize(3);
			inputHistory.inputs[0].updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::Inactive, GameplayTimestamp(0));
			inputHistory.inputs[0].updateAxis(GameplayInput::InputAxis::MoveHorizontal, 0.0f);
			inputHistory.inputs[1].updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::JustActivated, GameplayTimestamp(1));
			inputHistory.inputs[1].updateAxis(GameplayInput::InputAxis::MoveHorizontal, 0.5f);
			inputHistory.inputs[2].updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::Active, GameplayTimestamp(1));
			inputHistory.inputs[2].updateAxis(GameplayInput::InputAxis::MoveHorizontal, 1.0f);
			inputHistory.lastInputUpdateIdx = 3;
		}

		Network::ApplyPlayerInputMessage(serverGame->world, serverGame->stateRewinder, message, connectionId);

		const Input::InputHistory& inputHistory = serverGame->stateRewinder.getInputHistoryForClient(connectionId);

		ASSERT_EQ(inputHistory.inputs.size(), static_cast<size_t>(7));
		EXPECT_EQ(inputHistory.inputs[0].getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::Inactive);
		EXPECT_EQ(inputHistory.inputs[0].getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(0));
		EXPECT_FLOAT_EQ(inputHistory.inputs[0].getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 0.0f);
		EXPECT_EQ(inputHistory.inputs[1].getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::JustActivated);
		EXPECT_EQ(inputHistory.inputs[1].getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(1));
		EXPECT_FLOAT_EQ(inputHistory.inputs[1].getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 0.5f);
		EXPECT_EQ(inputHistory.inputs[2].getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::Active);
		EXPECT_EQ(inputHistory.inputs[2].getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(1));
		EXPECT_FLOAT_EQ(inputHistory.inputs[2].getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 1.0f);
		EXPECT_EQ(inputHistory.inputs[3].getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::Active);
		EXPECT_EQ(inputHistory.inputs[3].getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(1));
		EXPECT_FLOAT_EQ(inputHistory.inputs[3].getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 1.0f);
		EXPECT_EQ(inputHistory.inputs[4].getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::Active);
		EXPECT_EQ(inputHistory.inputs[4].getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(1));
		EXPECT_FLOAT_EQ(inputHistory.inputs[4].getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 1.0f);
		EXPECT_EQ(inputHistory.inputs[5].getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::JustDeactivated);
		EXPECT_EQ(inputHistory.inputs[5].getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(5));
		EXPECT_FLOAT_EQ(inputHistory.inputs[5].getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 0.75f);
		EXPECT_EQ(inputHistory.inputs[6].getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::Inactive);
		EXPECT_EQ(inputHistory.inputs[6].getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(5));
		EXPECT_FLOAT_EQ(inputHistory.inputs[6].getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 0.25f);
		EXPECT_EQ(inputHistory.lastInputUpdateIdx, static_cast<u32>(7));
		EXPECT_EQ(inputHistory.indexShift, static_cast<s32>(9));
	}
}
