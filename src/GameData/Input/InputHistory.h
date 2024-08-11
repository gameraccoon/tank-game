#pragma once

#include <limits>
#include <vector>

#include "EngineCommon/Types/BasicTypes.h"

#include "GameData/Input/GameplayInputFrameState.h"

namespace Input
{
	// how many frames of input we send per update
	constexpr size_t MAX_INPUT_HISTORY_SEND_SIZE = 10;

	struct InputHistory
	{
		std::vector<GameplayInput::FrameState> inputs;
		u32 lastInputUpdateIdx = 0;
		s32 indexShift = std::numeric_limits<s32>::max();
		unsigned int indexShiftIncorrectFrames = 0;
	};
}
