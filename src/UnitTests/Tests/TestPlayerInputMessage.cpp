#include "Base/precomp.h"

#include <gtest/gtest.h>
#include <memory>

#include "GameData/ComponentRegistration/ComponentFactoryRegistration.h"
#include "GameData/Components/ServerConnectionsComponent.generated.h"
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
		GameStateRewinder stateRewinder;
		WorldHolder worldHolder{&stateRewinder.getWorld(0), gameData};

		explicit TestGame(GameStateRewinder::HistoryType historyType)
			: stateRewinder(historyType, componentFactory, entityGenerator)
		{}
	};

	static std::unique_ptr<TestGame> CreateGameInstance(GameStateRewinder::HistoryType historyType)
	{
		std::unique_ptr<TestGame> testGame = std::make_unique<TestGame>(historyType);
		ComponentsRegistration::RegisterComponents(testGame->componentFactory);
		TimeComponent* time = testGame->world.getWorldComponents().addComponent<TimeComponent>();
		time->setValue(&testGame->stateRewinder.getTimeData());
		return testGame;
	}
}

TEST(PlayerInputMessage, SerializeAndDeserializeFirstInput_AllInputAdded)
{
	using namespace PlayerInputMessageInternal;
	auto clientGame = CreateGameInstance(GameStateRewinder::HistoryType::Client);

	{
		GameplayInput::FrameState frameState;
		frameState.updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::Inactive, GameplayTimestamp(0));
		frameState.updateAxis(GameplayInput::InputAxis::MoveHorizontal, 0.0f);
		clientGame->stateRewinder.setInputForUpdate(1u, frameState);
		frameState.updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::JustActivated, GameplayTimestamp(1));
		frameState.updateAxis(GameplayInput::InputAxis::MoveHorizontal, 0.5f);
		clientGame->stateRewinder.setInputForUpdate(2u, frameState);
		frameState.updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::Active, GameplayTimestamp(2));
		frameState.updateAxis(GameplayInput::InputAxis::MoveHorizontal, 1.0f);
		clientGame->stateRewinder.setInputForUpdate(3u, frameState);
		frameState.updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::JustDeactivated, GameplayTimestamp(3));
		frameState.updateAxis(GameplayInput::InputAxis::MoveHorizontal, 0.75f);
		clientGame->stateRewinder.setInputForUpdate(4u, frameState);
		frameState.updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::Inactive, GameplayTimestamp(4));
		frameState.updateAxis(GameplayInput::InputAxis::MoveHorizontal, 0.25f);
		clientGame->stateRewinder.setInputForUpdate(5u, frameState);
		clientGame->stateRewinder.getTimeData().lastFixedUpdateIndex = 5u;
	}

	HAL::ConnectionManager::Message message = Network::CreatePlayerInputMessage(clientGame->stateRewinder);

	{
		auto serverGame = CreateGameInstance(GameStateRewinder::HistoryType::Server);
		const ConnectionId connectionId = 1;
		serverGame->stateRewinder.getTimeData().lastFixedUpdateIndex = 6;
		Network::ApplyPlayerInputMessage(serverGame->world, serverGame->stateRewinder, message, connectionId);

		EXPECT_EQ(serverGame->stateRewinder.getLastKnownInputUpdateIdxForPlayer(connectionId), 7u);
		EXPECT_EQ(serverGame->stateRewinder.getLastKnownInputUpdateIdxForPlayers(std::vector<ConnectionId>{connectionId}), 7u);

		{
			const GameplayInput::FrameState& frame3Inputs = serverGame->stateRewinder.getPlayerInput(connectionId, 3);
			EXPECT_EQ(frame3Inputs.getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::Inactive);
			EXPECT_EQ(frame3Inputs.getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(0));
			EXPECT_FLOAT_EQ(frame3Inputs.getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 0.0f);
		}
		{
			const GameplayInput::FrameState& frame4Inputs = serverGame->stateRewinder.getPlayerInput(connectionId, 4);
			EXPECT_EQ(frame4Inputs.getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::JustActivated);
			EXPECT_EQ(frame4Inputs.getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(1));
			EXPECT_FLOAT_EQ(frame4Inputs.getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 0.5f);
		}
		{
			const GameplayInput::FrameState& frame5Inputs = serverGame->stateRewinder.getPlayerInput(connectionId, 5);
			EXPECT_EQ(frame5Inputs.getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::Active);
			EXPECT_EQ(frame5Inputs.getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(1));
			EXPECT_FLOAT_EQ(frame5Inputs.getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 1.0f);
		}
		{
			const GameplayInput::FrameState& frame6Inputs = serverGame->stateRewinder.getPlayerInput(connectionId, 6);
			EXPECT_EQ(frame6Inputs.getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::JustDeactivated);
			EXPECT_EQ(frame6Inputs.getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(3));
			EXPECT_FLOAT_EQ(frame6Inputs.getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 0.75f);
		}
		{
			const GameplayInput::FrameState& frame7Inputs = serverGame->stateRewinder.getPlayerInput(connectionId, 7);
			EXPECT_EQ(frame7Inputs.getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::Inactive);
			EXPECT_EQ(frame7Inputs.getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(3));
			EXPECT_FLOAT_EQ(frame7Inputs.getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 0.25f);
		}
		ServerConnectionsComponent* serverConnections = serverGame->stateRewinder.getNotRewindableComponents().getOrAddComponent<ServerConnectionsComponent>();
		ASSERT_EQ(serverConnections->getClientData().size(), 1u);
		EXPECT_EQ(serverConnections->getClientDataRef()[connectionId].indexShift, 2);
	}
}

TEST(PlayerInputMessage, SerializeAndDeserializePartlyKnownInput_NewInputsAdded)
{
	using namespace PlayerInputMessageInternal;
	auto clientGame = CreateGameInstance(GameStateRewinder::HistoryType::Client);

	{
		GameplayInput::FrameState frameState;
		frameState.updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::Inactive, GameplayTimestamp(0));
		frameState.updateAxis(GameplayInput::InputAxis::MoveHorizontal, 0.0f);
		clientGame->stateRewinder.setInputForUpdate(1u, frameState);
		frameState.updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::JustActivated, GameplayTimestamp(1));
		frameState.updateAxis(GameplayInput::InputAxis::MoveHorizontal, 0.5f);
		clientGame->stateRewinder.setInputForUpdate(2u, frameState);
		frameState.updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::Active, GameplayTimestamp(2));
		frameState.updateAxis(GameplayInput::InputAxis::MoveHorizontal, 1.0f);
		clientGame->stateRewinder.setInputForUpdate(3u, frameState);
		frameState.updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::JustDeactivated, GameplayTimestamp(3));
		frameState.updateAxis(GameplayInput::InputAxis::MoveHorizontal, 0.75f);
		clientGame->stateRewinder.setInputForUpdate(4u, frameState);
		frameState.updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::Inactive, GameplayTimestamp(4));
		frameState.updateAxis(GameplayInput::InputAxis::MoveHorizontal, 0.25f);
		clientGame->stateRewinder.setInputForUpdate(5u, frameState);
		clientGame->stateRewinder.getTimeData().lastFixedUpdateIndex = 5u;
	}

	HAL::ConnectionManager::Message message = Network::CreatePlayerInputMessage(clientGame->stateRewinder);

	{
		auto serverGame = CreateGameInstance(GameStateRewinder::HistoryType::Server);
		ConnectionId connectionId = 1;
		{
			{
				GameplayInput::FrameState frame1Inputs{};
				frame1Inputs.updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::Inactive, GameplayTimestamp(0));
				frame1Inputs.updateAxis(GameplayInput::InputAxis::MoveHorizontal, 0.0f);
				serverGame->stateRewinder.addPlayerInput(connectionId, 2, frame1Inputs);
			}
			{
				GameplayInput::FrameState frame2Inputs{};
				frame2Inputs.updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::JustActivated, GameplayTimestamp(1));
				frame2Inputs.updateAxis(GameplayInput::InputAxis::MoveHorizontal, 0.5f);
				serverGame->stateRewinder.addPlayerInput(connectionId, 3, frame2Inputs);
			}
			{
				GameplayInput::FrameState frame3Inputs{};
				frame3Inputs.updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::Active, GameplayTimestamp(1));
				frame3Inputs.updateAxis(GameplayInput::InputAxis::MoveHorizontal, 1.0f);
				serverGame->stateRewinder.addPlayerInput(connectionId, 4, frame3Inputs);
			}
			// fast-forward to frame 5, skipping inputs for frames 4 and 5
			serverGame->stateRewinder.getTimeData().lastFixedUpdateIndex = 5;
		}

		Network::ApplyPlayerInputMessage(serverGame->world, serverGame->stateRewinder, message, connectionId);

		EXPECT_EQ(serverGame->stateRewinder.getLastKnownInputUpdateIdxForPlayer(connectionId), 6u);
		EXPECT_EQ(serverGame->stateRewinder.getLastKnownInputUpdateIdxForPlayers(std::vector<ConnectionId>{connectionId}), 6u);

		{
			const GameplayInput::FrameState& frame2Inputs = serverGame->stateRewinder.getPlayerInput(connectionId, 2);
			EXPECT_EQ(frame2Inputs.getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::Inactive);
			EXPECT_EQ(frame2Inputs.getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(0));
			EXPECT_FLOAT_EQ(frame2Inputs.getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 0.0f);
		}
		{
			const GameplayInput::FrameState& frame3Inputs = serverGame->stateRewinder.getPlayerInput(connectionId, 3);
			EXPECT_EQ(frame3Inputs.getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::JustActivated);
			EXPECT_EQ(frame3Inputs.getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(1));
			EXPECT_FLOAT_EQ(frame3Inputs.getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 0.5f);
		}
		{
			const GameplayInput::FrameState& frame4Inputs = serverGame->stateRewinder.getPlayerInput(connectionId, 4);
			EXPECT_EQ(frame4Inputs.getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::Active);
			EXPECT_EQ(frame4Inputs.getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(1));
			EXPECT_FLOAT_EQ(frame4Inputs.getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 1.0f);
		}
		{
			const GameplayInput::FrameState& frame5Inputs = serverGame->stateRewinder.getPlayerInput(connectionId, 5);
			EXPECT_EQ(frame5Inputs.getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::JustDeactivated);
			EXPECT_EQ(frame5Inputs.getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(3));
			EXPECT_FLOAT_EQ(frame5Inputs.getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 0.75f);
		}
		{
			const GameplayInput::FrameState& frame6Inputs = serverGame->stateRewinder.getPlayerInput(connectionId, 6);
			EXPECT_EQ(frame6Inputs.getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::Inactive);
			EXPECT_EQ(frame6Inputs.getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(3));
			EXPECT_FLOAT_EQ(frame6Inputs.getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 0.25f);
		}

		ServerConnectionsComponent* serverConnections = serverGame->stateRewinder.getNotRewindableComponents().getOrAddComponent<ServerConnectionsComponent>();
		ASSERT_EQ(serverConnections->getClientData().size(), 1u);
		EXPECT_EQ(serverConnections->getClientDataRef()[connectionId].indexShift, 1);
	}
}

TEST(PlayerInputMessage, SerializeAndDeserializeInputWithAGap_NewInputsAddedMissedInputsPredicted)
{
	using namespace PlayerInputMessageInternal;
	auto clientGame = CreateGameInstance(GameStateRewinder::HistoryType::Client);

	{
		GameplayInput::FrameState frameState;
		frameState.updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::Inactive, GameplayTimestamp(0));
		frameState.updateAxis(GameplayInput::InputAxis::MoveHorizontal, 0.0f);
		clientGame->stateRewinder.setInputForUpdate(1u, frameState);
		frameState.updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::JustActivated, GameplayTimestamp(1));
		frameState.updateAxis(GameplayInput::InputAxis::MoveHorizontal, 0.5f);
		clientGame->stateRewinder.setInputForUpdate(2u, frameState);
		frameState.updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::Active, GameplayTimestamp(2));
		frameState.updateAxis(GameplayInput::InputAxis::MoveHorizontal, 1.0f);
		clientGame->stateRewinder.setInputForUpdate(3u, frameState);
		frameState.updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::Active, GameplayTimestamp(3));
		frameState.updateAxis(GameplayInput::InputAxis::MoveHorizontal, 1.0f);
		clientGame->stateRewinder.setInputForUpdate(4u, frameState);
		frameState.updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::Active, GameplayTimestamp(4));
		frameState.updateAxis(GameplayInput::InputAxis::MoveHorizontal, 1.0f);
		clientGame->stateRewinder.setInputForUpdate(5u, frameState);
		frameState.updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::JustDeactivated, GameplayTimestamp(5));
		frameState.updateAxis(GameplayInput::InputAxis::MoveHorizontal, 0.75f);
		clientGame->stateRewinder.setInputForUpdate(6u, frameState);
		frameState.updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::Inactive, GameplayTimestamp(6));
		frameState.updateAxis(GameplayInput::InputAxis::MoveHorizontal, 0.25f);
		clientGame->stateRewinder.setInputForUpdate(7u, frameState);
		clientGame->stateRewinder.getTimeData().lastFixedUpdateIndex = 7u;
	}

	// keep only two inputs from above
	clientGame->stateRewinder.trimOldFrames(6u);
	HAL::ConnectionManager::Message message = Network::CreatePlayerInputMessage(clientGame->stateRewinder);

	{
		auto serverGame = CreateGameInstance(GameStateRewinder::HistoryType::Server);
		ConnectionId connectionId = 1;
		{
			{
				GameplayInput::FrameState frame1Inputs{};
				frame1Inputs.updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::Inactive, GameplayTimestamp(0));
				frame1Inputs.updateAxis(GameplayInput::InputAxis::MoveHorizontal, 0.0f);
				serverGame->stateRewinder.addPlayerInput(connectionId, 2u, frame1Inputs);
			}
			{
				GameplayInput::FrameState frame2Inputs{};
				frame2Inputs.updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::JustActivated, GameplayTimestamp(1));
				frame2Inputs.updateAxis(GameplayInput::InputAxis::MoveHorizontal, 0.5f);
				serverGame->stateRewinder.addPlayerInput(connectionId, 3u, frame2Inputs);
			}
			{
				GameplayInput::FrameState frame3Inputs{};
				frame3Inputs.updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::Active, GameplayTimestamp(1));
				frame3Inputs.updateAxis(GameplayInput::InputAxis::MoveHorizontal, 1.0f);
				serverGame->stateRewinder.addPlayerInput(connectionId, 4u, frame3Inputs);
			}
			serverGame->stateRewinder.getTimeData().lastFixedUpdateIndex = 7;
		}

		Network::ApplyPlayerInputMessage(serverGame->world, serverGame->stateRewinder, message, connectionId);

		EXPECT_EQ(serverGame->stateRewinder.getLastKnownInputUpdateIdxForPlayer(connectionId), 8u);
		EXPECT_EQ(serverGame->stateRewinder.getLastKnownInputUpdateIdxForPlayers(std::vector<ConnectionId>{connectionId}), 8u);

		{
			const GameplayInput::FrameState& frame2Inputs = serverGame->stateRewinder.getPlayerInput(connectionId, 2u);
			EXPECT_EQ(frame2Inputs.getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::Inactive);
			EXPECT_EQ(frame2Inputs.getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(0));
			EXPECT_FLOAT_EQ(frame2Inputs.getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 0.0f);
		}
		{
			const GameplayInput::FrameState& frame3Inputs = serverGame->stateRewinder.getPlayerInput(connectionId, 3u);
			EXPECT_EQ(frame3Inputs.getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::JustActivated);
			EXPECT_EQ(frame3Inputs.getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(1));
			EXPECT_FLOAT_EQ(frame3Inputs.getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 0.5f);
		}
		{
			const GameplayInput::FrameState& frame4Inputs = serverGame->stateRewinder.getPlayerInput(connectionId, 4u);
			EXPECT_EQ(frame4Inputs.getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::Active);
			EXPECT_EQ(frame4Inputs.getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(1));
			EXPECT_FLOAT_EQ(frame4Inputs.getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 1.0f);
		}
		{
			const GameplayInput::FrameState& frame5Inputs = serverGame->stateRewinder.getPlayerInput(connectionId, 5u);
			EXPECT_EQ(frame5Inputs.getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::Active);
			EXPECT_EQ(frame5Inputs.getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(1));
			EXPECT_FLOAT_EQ(frame5Inputs.getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 1.0f);
		}
		{
			const GameplayInput::FrameState& frame6Inputs = serverGame->stateRewinder.getPlayerInput(connectionId, 6u);
			EXPECT_EQ(frame6Inputs.getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::Active);
			EXPECT_EQ(frame6Inputs.getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(1));
			EXPECT_FLOAT_EQ(frame6Inputs.getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 1.0f);
		}
		{
			const GameplayInput::FrameState& frame7Inputs = serverGame->stateRewinder.getPlayerInput(connectionId, 7u);
			EXPECT_EQ(frame7Inputs.getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::JustDeactivated);
			EXPECT_EQ(frame7Inputs.getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(5));
			EXPECT_FLOAT_EQ(frame7Inputs.getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 0.75f);
		}
		{
			const GameplayInput::FrameState& frame8Inputs = serverGame->stateRewinder.getPlayerInput(connectionId, 8u);
			EXPECT_EQ(frame8Inputs.getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::Inactive);
			EXPECT_EQ(frame8Inputs.getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(5));
			EXPECT_FLOAT_EQ(frame8Inputs.getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 0.25f);
		}

		ServerConnectionsComponent* serverConnections = serverGame->stateRewinder.getNotRewindableComponents().getOrAddComponent<ServerConnectionsComponent>();
		ASSERT_EQ(serverConnections->getClientData().size(), 1u);
		EXPECT_EQ(serverConnections->getClientDataRef()[connectionId].indexShift, 1);
	}
}
