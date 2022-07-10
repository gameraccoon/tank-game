#include "Base/precomp.h"

#include <gtest/gtest.h>
#include <memory>

#include "GameData/ComponentRegistration/ComponentFactoryRegistration.h"
#include "GameData/Components/InputHistoryComponent.generated.h"
#include "GameData/Components/NetworkIdComponent.generated.h"
#include "GameData/Components/NetworkIdMappingComponent.generated.h"
#include "GameData/Components/ServerConnectionsComponent.generated.h"
#include "GameData/Components/TimeComponent.generated.h"
#include "GameData/Input/GameplayInput.h"
#include "GameData/World.h"

#include "Utils/Network/Messages/PlayerInputMessage.h"


namespace PlayerInputMessageInternal
{
	struct TestGame
	{
		ComponentFactory componentFactory;
		RaccoonEcs::IncrementalEntityGenerator entityGenerator;
		World world{componentFactory, entityGenerator};
	};

	static std::unique_ptr<TestGame> CreateGameInstance()
	{
		std::unique_ptr<TestGame> world = std::make_unique<TestGame>();
		ComponentsRegistration::RegisterComponents(world->componentFactory);
		return world;
	}
}

TEST(PlayerInputMessage, SerializeAndDeserializeFirstInput)
{
	using namespace PlayerInputMessageInternal;
	auto clientGame = CreateGameInstance();

	{
		InputHistoryComponent* inputHistory = clientGame->world.getNotRewindableWorldComponents().getOrAddComponent<InputHistoryComponent>();
		inputHistory->getInputsRef().resize(5);
		inputHistory->getInputsRef()[0].updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::Inactive, GameplayTimestamp(0));
		inputHistory->getInputsRef()[0].updateAxis(GameplayInput::InputAxis::MoveHorizontal, 0.0f);
		inputHistory->getInputsRef()[1].updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::JustActivated, GameplayTimestamp(1));
		inputHistory->getInputsRef()[1].updateAxis(GameplayInput::InputAxis::MoveHorizontal, 0.5f);
		inputHistory->getInputsRef()[2].updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::Active, GameplayTimestamp(2));
		inputHistory->getInputsRef()[2].updateAxis(GameplayInput::InputAxis::MoveHorizontal, 1.0f);
		inputHistory->getInputsRef()[3].updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::JustDeactivated, GameplayTimestamp(3));
		inputHistory->getInputsRef()[3].updateAxis(GameplayInput::InputAxis::MoveHorizontal, 0.75f);
		inputHistory->getInputsRef()[4].updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::Inactive, GameplayTimestamp(4));
		inputHistory->getInputsRef()[4].updateAxis(GameplayInput::InputAxis::MoveHorizontal, 0.25f);
		inputHistory->setLastInputUpdateIdx(4);
	}

	HAL::ConnectionManager::Message message = Network::CreatePlayerInputMessage(clientGame->world);

	{
		auto serverGame = CreateGameInstance();
		ConnectionId connectionId = 1;
		{
			ServerConnectionsComponent* serverConnections = serverGame->world.getNotRewindableWorldComponents().getOrAddComponent<ServerConnectionsComponent>();
			serverConnections->getInputsRef()[connectionId];
			TimeComponent* time = serverGame->world.getWorldComponents().addComponent<TimeComponent>();
			time->getValueRef().lastFixedUpdateIndex = 6;
		}
		Network::ApplyPlayerInputMessage(serverGame->world, std::move(message), connectionId);

		ServerConnectionsComponent* serverConnections = serverGame->world.getNotRewindableWorldComponents().getOrAddComponent<ServerConnectionsComponent>();
		const Input::InputHistory& inputHistory = serverConnections->getInputs().at(connectionId);

		ASSERT_EQ(inputHistory.inputs.size(), static_cast<size_t>(5));
		EXPECT_EQ(inputHistory.inputs[0].getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::Inactive);
		EXPECT_EQ(inputHistory.inputs[0].getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(0));
		EXPECT_FLOAT_EQ(inputHistory.inputs[0].getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 0.0f);
		EXPECT_EQ(inputHistory.inputs[1].getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::JustActivated);
		EXPECT_EQ(inputHistory.inputs[1].getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(1));
		EXPECT_FLOAT_EQ(inputHistory.inputs[1].getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 0.5f);
		EXPECT_EQ(inputHistory.inputs[2].getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::Active);
		EXPECT_EQ(inputHistory.inputs[2].getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(2));
		EXPECT_FLOAT_EQ(inputHistory.inputs[2].getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 1.0f);
		EXPECT_EQ(inputHistory.inputs[3].getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::JustDeactivated);
		EXPECT_EQ(inputHistory.inputs[3].getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(3));
		EXPECT_FLOAT_EQ(inputHistory.inputs[3].getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 0.75f);
		EXPECT_EQ(inputHistory.inputs[4].getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::Inactive);
		EXPECT_EQ(inputHistory.inputs[4].getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(4));
		EXPECT_FLOAT_EQ(inputHistory.inputs[4].getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 0.25f);
		EXPECT_EQ(inputHistory.lastInputUpdateIdx, static_cast<u32>(4));
		EXPECT_EQ(inputHistory.indexShift, static_cast<s32>(3));
	}
}

TEST(PlayerInputMessage, SerializeAndDeserializePartlyKnownInput)
{
	using namespace PlayerInputMessageInternal;
	auto clientGame = CreateGameInstance();

	{
		InputHistoryComponent* inputHistory = clientGame->world.getNotRewindableWorldComponents().getOrAddComponent<InputHistoryComponent>();
		inputHistory->getInputsRef().resize(5);
		inputHistory->getInputsRef()[0].updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::Inactive, GameplayTimestamp(0));
		inputHistory->getInputsRef()[0].updateAxis(GameplayInput::InputAxis::MoveHorizontal, 0.0f);
		inputHistory->getInputsRef()[1].updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::JustActivated, GameplayTimestamp(1));
		inputHistory->getInputsRef()[1].updateAxis(GameplayInput::InputAxis::MoveHorizontal, 0.5f);
		inputHistory->getInputsRef()[2].updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::Active, GameplayTimestamp(2));
		inputHistory->getInputsRef()[2].updateAxis(GameplayInput::InputAxis::MoveHorizontal, 1.0f);
		inputHistory->getInputsRef()[3].updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::JustDeactivated, GameplayTimestamp(3));
		inputHistory->getInputsRef()[3].updateAxis(GameplayInput::InputAxis::MoveHorizontal, 0.75f);
		inputHistory->getInputsRef()[4].updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::Inactive, GameplayTimestamp(4));
		inputHistory->getInputsRef()[4].updateAxis(GameplayInput::InputAxis::MoveHorizontal, 0.25f);
		inputHistory->setLastInputUpdateIdx(4);
	}

	HAL::ConnectionManager::Message message = Network::CreatePlayerInputMessage(clientGame->world);

	{
		auto serverGame = CreateGameInstance();
		ConnectionId connectionId = 1;
		{
			ServerConnectionsComponent* serverConnections = serverGame->world.getNotRewindableWorldComponents().getOrAddComponent<ServerConnectionsComponent>();
			serverConnections->getInputsRef()[connectionId];

			Input::InputHistory& inputHistory = serverConnections->getInputsRef().at(connectionId);
			inputHistory.inputs.resize(3);
			inputHistory.inputs[0].updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::Inactive, GameplayTimestamp(0));
			inputHistory.inputs[0].updateAxis(GameplayInput::InputAxis::MoveHorizontal, 0.0f);
			inputHistory.inputs[1].updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::JustActivated, GameplayTimestamp(1));
			inputHistory.inputs[1].updateAxis(GameplayInput::InputAxis::MoveHorizontal, 0.5f);
			inputHistory.inputs[2].updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::Active, GameplayTimestamp(2));
			inputHistory.inputs[2].updateAxis(GameplayInput::InputAxis::MoveHorizontal, 1.0f);
			inputHistory.lastInputUpdateIdx = 2;
			TimeComponent* time = serverGame->world.getWorldComponents().addComponent<TimeComponent>();
			time->getValueRef().lastFixedUpdateIndex = 6;
		}

		Network::ApplyPlayerInputMessage(serverGame->world, std::move(message), connectionId);

		ServerConnectionsComponent* serverConnections = serverGame->world.getNotRewindableWorldComponents().getOrAddComponent<ServerConnectionsComponent>();
		const Input::InputHistory& inputHistory = serverConnections->getInputs().at(connectionId);

		ASSERT_EQ(inputHistory.inputs.size(), static_cast<size_t>(5));
		EXPECT_EQ(inputHistory.inputs[0].getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::Inactive);
		EXPECT_EQ(inputHistory.inputs[0].getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(0));
		EXPECT_FLOAT_EQ(inputHistory.inputs[0].getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 0.0f);
		EXPECT_EQ(inputHistory.inputs[1].getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::JustActivated);
		EXPECT_EQ(inputHistory.inputs[1].getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(1));
		EXPECT_FLOAT_EQ(inputHistory.inputs[1].getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 0.5f);
		EXPECT_EQ(inputHistory.inputs[2].getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::Active);
		EXPECT_EQ(inputHistory.inputs[2].getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(2));
		EXPECT_FLOAT_EQ(inputHistory.inputs[2].getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 1.0f);
		EXPECT_EQ(inputHistory.inputs[3].getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::JustDeactivated);
		EXPECT_EQ(inputHistory.inputs[3].getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(3));
		EXPECT_FLOAT_EQ(inputHistory.inputs[3].getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 0.75f);
		EXPECT_EQ(inputHistory.inputs[4].getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::Inactive);
		EXPECT_EQ(inputHistory.inputs[4].getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(4));
		EXPECT_FLOAT_EQ(inputHistory.inputs[4].getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 0.25f);
		EXPECT_EQ(inputHistory.lastInputUpdateIdx, static_cast<u32>(4));
		EXPECT_EQ(inputHistory.indexShift, static_cast<s32>(3));
	}
}

TEST(PlayerInputMessage, SerializeAndDeserializeInputWithAGap)
{
	using namespace PlayerInputMessageInternal;
	auto clientGame = CreateGameInstance();

	{
		InputHistoryComponent* inputHistory = clientGame->world.getNotRewindableWorldComponents().getOrAddComponent<InputHistoryComponent>();
		inputHistory->getInputsRef().resize(2);
		inputHistory->getInputsRef()[0].updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::JustDeactivated, GameplayTimestamp(3));
		inputHistory->getInputsRef()[0].updateAxis(GameplayInput::InputAxis::MoveHorizontal, 0.75f);
		inputHistory->getInputsRef()[1].updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::Inactive, GameplayTimestamp(4));
		inputHistory->getInputsRef()[1].updateAxis(GameplayInput::InputAxis::MoveHorizontal, 0.25f);
		inputHistory->setLastInputUpdateIdx(46);
	}

	HAL::ConnectionManager::Message message = Network::CreatePlayerInputMessage(clientGame->world);

	{
		auto serverGame = CreateGameInstance();
		ConnectionId connectionId = 1;
		{
			ServerConnectionsComponent* serverConnections = serverGame->world.getNotRewindableWorldComponents().getOrAddComponent<ServerConnectionsComponent>();
			serverConnections->getInputsRef()[connectionId];

			Input::InputHistory& inputHistory = serverConnections->getInputsRef().at(connectionId);

			inputHistory.inputs.resize(3);
			inputHistory.inputs[0].updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::Inactive, GameplayTimestamp(0));
			inputHistory.inputs[0].updateAxis(GameplayInput::InputAxis::MoveHorizontal, 0.0f);
			inputHistory.inputs[1].updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::JustActivated, GameplayTimestamp(1));
			inputHistory.inputs[1].updateAxis(GameplayInput::InputAxis::MoveHorizontal, 0.5f);
			inputHistory.inputs[2].updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::Active, GameplayTimestamp(2));
			inputHistory.inputs[2].updateAxis(GameplayInput::InputAxis::MoveHorizontal, 1.0f);
			inputHistory.lastInputUpdateIdx = 42;
			TimeComponent* time = serverGame->world.getWorldComponents().addComponent<TimeComponent>();
			time->getValueRef().lastFixedUpdateIndex = 48;
		}

		Network::ApplyPlayerInputMessage(serverGame->world, std::move(message), connectionId);

		ServerConnectionsComponent* serverConnections = serverGame->world.getNotRewindableWorldComponents().getOrAddComponent<ServerConnectionsComponent>();
		const Input::InputHistory& inputHistory = serverConnections->getInputs().at(connectionId);

		ASSERT_EQ(inputHistory.inputs.size(), static_cast<size_t>(7));
		EXPECT_EQ(inputHistory.inputs[0].getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::Inactive);
		EXPECT_EQ(inputHistory.inputs[0].getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(0));
		EXPECT_FLOAT_EQ(inputHistory.inputs[0].getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 0.0f);
		EXPECT_EQ(inputHistory.inputs[1].getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::JustActivated);
		EXPECT_EQ(inputHistory.inputs[1].getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(1));
		EXPECT_FLOAT_EQ(inputHistory.inputs[1].getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 0.5f);
		EXPECT_EQ(inputHistory.inputs[2].getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::Active);
		EXPECT_EQ(inputHistory.inputs[2].getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(2));
		EXPECT_FLOAT_EQ(inputHistory.inputs[2].getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 1.0f);
		EXPECT_EQ(inputHistory.inputs[3].getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::Active);
		EXPECT_EQ(inputHistory.inputs[3].getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(2));
		EXPECT_FLOAT_EQ(inputHistory.inputs[3].getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 1.0f);
		EXPECT_EQ(inputHistory.inputs[4].getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::Active);
		EXPECT_EQ(inputHistory.inputs[4].getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(2));
		EXPECT_FLOAT_EQ(inputHistory.inputs[4].getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 1.0f);
		EXPECT_EQ(inputHistory.inputs[5].getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::JustDeactivated);
		EXPECT_EQ(inputHistory.inputs[5].getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(3));
		EXPECT_FLOAT_EQ(inputHistory.inputs[5].getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 0.75f);
		EXPECT_EQ(inputHistory.inputs[6].getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::Inactive);
		EXPECT_EQ(inputHistory.inputs[6].getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(4));
		EXPECT_FLOAT_EQ(inputHistory.inputs[6].getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 0.25f);
		EXPECT_EQ(inputHistory.lastInputUpdateIdx, static_cast<u32>(46));
		EXPECT_EQ(inputHistory.indexShift, static_cast<s32>(3));
	}
}
