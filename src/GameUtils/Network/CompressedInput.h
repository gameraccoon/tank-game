#pragma once

#include "GameData/Input/GameplayInputFrameState.h"

namespace Utils
{
	void AppendInputHistory(std::vector<std::byte>& stream, const std::vector<GameplayInput::FrameState>& inputs, size_t inputsToSend);
	std::vector<GameplayInput::FrameState> ReadInputHistory(const std::vector<std::byte>& stream, size_t inputsToRead, size_t& streamIndex);
}
