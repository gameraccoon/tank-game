#pragma once

#include <span>

#include "GameData/Input/GameplayInputFrameState.h"

namespace Utils
{
	void AppendInputHistory(std::vector<std::byte>& stream, const std::vector<GameplayInput::FrameState>& inputs, size_t inputsToSend);
	std::vector<GameplayInput::FrameState> ReadInputHistory(std::span<const std::byte> stream, size_t inputsToRead, size_t& streamIndex);
}
