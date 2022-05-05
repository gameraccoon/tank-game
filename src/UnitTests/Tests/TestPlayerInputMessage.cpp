#include "Base/precomp.h"

#include <gtest/gtest.h>
#include <memory>

#include "GameData/ComponentRegistration/ComponentFactoryRegistration.h"
#include "GameData/Components/InputHistoryComponent.generated.h"
#include "GameData/Components/NetworkIdComponent.generated.h"
#include "GameData/Components/NetworkIdMappingComponent.generated.h"
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
		Entity entity = clientGame->world.getEntityManager().addEntity();
		InputHistoryComponent* inputHistory = clientGame->world.getEntityManager().addComponent<InputHistoryComponent>(entity);
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
		inputHistory->setLastInputFrameIdx(4);

		NetworkIdComponent* networkId = clientGame->world.getEntityManager().addComponent<NetworkIdComponent>(entity);
		networkId->setId(1);
	}

	HAL::ConnectionManager::Message message = Network::CreatePlayerInputMessage(clientGame->world.getEntityManager());

	{
		auto serverGame = CreateGameInstance();
		Entity entity = serverGame->world.getEntityManager().addEntity();
		serverGame->world.getEntityManager().addComponent<InputHistoryComponent>(entity);
		NetworkIdMappingComponent* networkIdMapping = serverGame->world.getWorldComponents().getOrAddComponent<NetworkIdMappingComponent>();
		networkIdMapping->getNetworkIdToEntityRef().emplace(1,  entity);

		Network::ApplyPlayerInputMessage(serverGame->world, std::move(message));

		auto [inputHistory] = serverGame->world.getEntityManager().getEntityComponents<InputHistoryComponent>(entity);

		ASSERT_EQ(inputHistory->getInputs().size(), static_cast<size_t>(5));
		EXPECT_EQ(inputHistory->getInputs()[0].getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::Inactive);
		EXPECT_EQ(inputHistory->getInputs()[0].getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(0));
		EXPECT_FLOAT_EQ(inputHistory->getInputs()[0].getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 0.0f);
		EXPECT_EQ(inputHistory->getInputs()[1].getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::JustActivated);
		EXPECT_EQ(inputHistory->getInputs()[1].getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(1));
		EXPECT_FLOAT_EQ(inputHistory->getInputs()[1].getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 0.5f);
		EXPECT_EQ(inputHistory->getInputs()[2].getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::Active);
		EXPECT_EQ(inputHistory->getInputs()[2].getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(2));
		EXPECT_FLOAT_EQ(inputHistory->getInputs()[2].getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 1.0f);
		EXPECT_EQ(inputHistory->getInputs()[3].getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::JustDeactivated);
		EXPECT_EQ(inputHistory->getInputs()[3].getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(3));
		EXPECT_FLOAT_EQ(inputHistory->getInputs()[3].getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 0.75f);
		EXPECT_EQ(inputHistory->getInputs()[4].getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::Inactive);
		EXPECT_EQ(inputHistory->getInputs()[4].getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(4));
		EXPECT_FLOAT_EQ(inputHistory->getInputs()[4].getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 0.25f);
	}
}

TEST(PlayerInputMessage, SerializeAndDeserializePartlyKnownInput)
{
	using namespace PlayerInputMessageInternal;
	auto clientGame = CreateGameInstance();

	{
		Entity entity = clientGame->world.getEntityManager().addEntity();
		InputHistoryComponent* inputHistory = clientGame->world.getEntityManager().addComponent<InputHistoryComponent>(entity);
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
		inputHistory->setLastInputFrameIdx(4);

		NetworkIdComponent* networkId = clientGame->world.getEntityManager().addComponent<NetworkIdComponent>(entity);
		networkId->setId(1);
	}

	HAL::ConnectionManager::Message message = Network::CreatePlayerInputMessage(clientGame->world.getEntityManager());

	{
		auto serverGame = CreateGameInstance();
		Entity entity = serverGame->world.getEntityManager().addEntity();
		{
			InputHistoryComponent* inputHistory = serverGame->world.getEntityManager().addComponent<InputHistoryComponent>(entity);
			inputHistory->getInputsRef().resize(3);
			inputHistory->getInputsRef()[0].updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::Inactive, GameplayTimestamp(0));
			inputHistory->getInputsRef()[0].updateAxis(GameplayInput::InputAxis::MoveHorizontal, 0.0f);
			inputHistory->getInputsRef()[1].updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::JustActivated, GameplayTimestamp(1));
			inputHistory->getInputsRef()[1].updateAxis(GameplayInput::InputAxis::MoveHorizontal, 0.5f);
			inputHistory->getInputsRef()[2].updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::Active, GameplayTimestamp(2));
			inputHistory->getInputsRef()[2].updateAxis(GameplayInput::InputAxis::MoveHorizontal, 1.0f);
			inputHistory->setLastInputFrameIdx(2);
		}

		NetworkIdMappingComponent* networkIdMapping = serverGame->world.getWorldComponents().getOrAddComponent<NetworkIdMappingComponent>();
		networkIdMapping->getNetworkIdToEntityRef().emplace(1,  entity);

		Network::ApplyPlayerInputMessage(serverGame->world, std::move(message));

		auto [inputHistory] = serverGame->world.getEntityManager().getEntityComponents<InputHistoryComponent>(entity);

		ASSERT_EQ(inputHistory->getInputs().size(), static_cast<size_t>(5));
		EXPECT_EQ(inputHistory->getInputs()[0].getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::Inactive);
		EXPECT_EQ(inputHistory->getInputs()[0].getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(0));
		EXPECT_FLOAT_EQ(inputHistory->getInputs()[0].getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 0.0f);
		EXPECT_EQ(inputHistory->getInputs()[1].getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::JustActivated);
		EXPECT_EQ(inputHistory->getInputs()[1].getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(1));
		EXPECT_FLOAT_EQ(inputHistory->getInputs()[1].getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 0.5f);
		EXPECT_EQ(inputHistory->getInputs()[2].getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::Active);
		EXPECT_EQ(inputHistory->getInputs()[2].getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(2));
		EXPECT_FLOAT_EQ(inputHistory->getInputs()[2].getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 1.0f);
		EXPECT_EQ(inputHistory->getInputs()[3].getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::JustDeactivated);
		EXPECT_EQ(inputHistory->getInputs()[3].getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(3));
		EXPECT_FLOAT_EQ(inputHistory->getInputs()[3].getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 0.75f);
		EXPECT_EQ(inputHistory->getInputs()[4].getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::Inactive);
		EXPECT_EQ(inputHistory->getInputs()[4].getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(4));
		EXPECT_FLOAT_EQ(inputHistory->getInputs()[4].getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 0.25f);
	}
}

TEST(PlayerInputMessage, SerializeAndDeserializeInputWithAGap)
{
	using namespace PlayerInputMessageInternal;
	auto clientGame = CreateGameInstance();

	{
		Entity entity = clientGame->world.getEntityManager().addEntity();
		InputHistoryComponent* inputHistory = clientGame->world.getEntityManager().addComponent<InputHistoryComponent>(entity);
		inputHistory->getInputsRef().resize(2);
		inputHistory->getInputsRef()[0].updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::JustDeactivated, GameplayTimestamp(3));
		inputHistory->getInputsRef()[0].updateAxis(GameplayInput::InputAxis::MoveHorizontal, 0.75f);
		inputHistory->getInputsRef()[1].updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::Inactive, GameplayTimestamp(4));
		inputHistory->getInputsRef()[1].updateAxis(GameplayInput::InputAxis::MoveHorizontal, 0.25f);
		inputHistory->setLastInputFrameIdx(6);

		NetworkIdComponent* networkId = clientGame->world.getEntityManager().addComponent<NetworkIdComponent>(entity);
		networkId->setId(1);
	}

	HAL::ConnectionManager::Message message = Network::CreatePlayerInputMessage(clientGame->world.getEntityManager());

	{
		auto serverGame = CreateGameInstance();
		Entity entity = serverGame->world.getEntityManager().addEntity();
		{
			InputHistoryComponent* inputHistory = serverGame->world.getEntityManager().addComponent<InputHistoryComponent>(entity);
			inputHistory->getInputsRef().resize(3);
			inputHistory->getInputsRef()[0].updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::Inactive, GameplayTimestamp(0));
			inputHistory->getInputsRef()[0].updateAxis(GameplayInput::InputAxis::MoveHorizontal, 0.0f);
			inputHistory->getInputsRef()[1].updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::JustActivated, GameplayTimestamp(1));
			inputHistory->getInputsRef()[1].updateAxis(GameplayInput::InputAxis::MoveHorizontal, 0.5f);
			inputHistory->getInputsRef()[2].updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::Active, GameplayTimestamp(2));
			inputHistory->getInputsRef()[2].updateAxis(GameplayInput::InputAxis::MoveHorizontal, 1.0f);
			inputHistory->setLastInputFrameIdx(2);
		}

		NetworkIdMappingComponent* networkIdMapping = serverGame->world.getWorldComponents().getOrAddComponent<NetworkIdMappingComponent>();
		networkIdMapping->getNetworkIdToEntityRef().emplace(1,  entity);

		Network::ApplyPlayerInputMessage(serverGame->world, std::move(message));

		auto [inputHistory] = serverGame->world.getEntityManager().getEntityComponents<InputHistoryComponent>(entity);

		ASSERT_EQ(inputHistory->getInputs().size(), static_cast<size_t>(7));
		EXPECT_EQ(inputHistory->getInputs()[0].getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::Inactive);
		EXPECT_EQ(inputHistory->getInputs()[0].getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(0));
		EXPECT_FLOAT_EQ(inputHistory->getInputs()[0].getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 0.0f);
		EXPECT_EQ(inputHistory->getInputs()[1].getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::JustActivated);
		EXPECT_EQ(inputHistory->getInputs()[1].getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(1));
		EXPECT_FLOAT_EQ(inputHistory->getInputs()[1].getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 0.5f);
		EXPECT_EQ(inputHistory->getInputs()[2].getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::Active);
		EXPECT_EQ(inputHistory->getInputs()[2].getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(2));
		EXPECT_FLOAT_EQ(inputHistory->getInputs()[2].getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 1.0f);
		EXPECT_EQ(inputHistory->getInputs()[3].getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::Active);
		EXPECT_EQ(inputHistory->getInputs()[3].getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(2));
		EXPECT_FLOAT_EQ(inputHistory->getInputs()[3].getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 1.0f);
		EXPECT_EQ(inputHistory->getInputs()[4].getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::Active);
		EXPECT_EQ(inputHistory->getInputs()[4].getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(2));
		EXPECT_FLOAT_EQ(inputHistory->getInputs()[4].getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 1.0f);
		EXPECT_EQ(inputHistory->getInputs()[5].getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::JustDeactivated);
		EXPECT_EQ(inputHistory->getInputs()[5].getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(3));
		EXPECT_FLOAT_EQ(inputHistory->getInputs()[5].getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 0.75f);
		EXPECT_EQ(inputHistory->getInputs()[6].getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::Inactive);
		EXPECT_EQ(inputHistory->getInputs()[6].getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(4));
		EXPECT_FLOAT_EQ(inputHistory->getInputs()[6].getAxisValue(GameplayInput::InputAxis::MoveHorizontal), 0.25f);
	}
}
