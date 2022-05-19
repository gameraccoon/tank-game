#pragma once

#include <limits>
#include <vector>

#include "Base/Types/BasicTypes.h"
#include "GameData/Input/GameplayInput.h"

namespace Input
{
	struct InputHistory
	{
		std::vector<GameplayInput::FrameState> inputs;
		u32 lastInputUpdateIdx = std::numeric_limits<u32>::max();
	};
}
