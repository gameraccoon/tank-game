#include "Base/precomp.h"

#include <gtest/gtest.h>

#include "Utils/Network/CompressedInput.h"

TEST(CompressedInput, WriteAndReadEmptyInput)
{
	std::vector<GameplayInput::FrameState> originalStates;
	originalStates.resize(10);
	std::vector<std::byte> stream;

	Utils::WriteInputHistory(stream, originalStates, 10);

	size_t streamIndex = 0;
	std::vector<GameplayInput::FrameState> newStates = Utils::ReadInputHistory(stream, 10, streamIndex);

	EXPECT_EQ(newStates, originalStates);
}

TEST(CompressedInput, WriteAndReadTestInput)
{
	std::vector<GameplayInput::FrameState> originalStates;
	originalStates.resize(5);
	originalStates[0].updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::Inactive, GameplayTimestamp(0));
	originalStates[1].updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::JustActivated, GameplayTimestamp(1));
	originalStates[2].updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::Active, GameplayTimestamp(2));
	originalStates[3].updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::JustDeactivated, GameplayTimestamp(3));
	originalStates[4].updateKey(GameplayInput::InputKey::Shoot, GameplayInput::KeyState::Inactive, GameplayTimestamp(4));
	std::vector<std::byte> stream;

	Utils::WriteInputHistory(stream, originalStates, 5);

	size_t streamIndex = 0;
	std::vector<GameplayInput::FrameState> newStates = Utils::ReadInputHistory(stream, 5, streamIndex);

	EXPECT_EQ(newStates, originalStates);
	EXPECT_EQ(newStates[0].getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::Inactive);
	EXPECT_EQ(newStates[0].getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(0));
	EXPECT_EQ(newStates[1].getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::JustActivated);
	EXPECT_EQ(newStates[1].getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(1));
	EXPECT_EQ(newStates[2].getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::Active);
	EXPECT_EQ(newStates[2].getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(2));
	EXPECT_EQ(newStates[3].getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::JustDeactivated);
	EXPECT_EQ(newStates[3].getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(3));
	EXPECT_EQ(newStates[4].getKeyState(GameplayInput::InputKey::Shoot), GameplayInput::KeyState::Inactive);
	EXPECT_EQ(newStates[4].getLastFlipTime(GameplayInput::InputKey::Shoot), GameplayTimestamp(4));
}
