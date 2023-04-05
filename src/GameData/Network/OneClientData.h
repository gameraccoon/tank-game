#pragma once

#include "GameData/EcsDefinitions.h"

struct OneClientData
{
	OptionalEntity playerEntity;
	// difference between client and server update indices
	s32 indexShift = std::numeric_limits<s32>::max();
	// for how many frames in a row indexShift drifted
	unsigned int indexShiftIncorrectFrames = 0;
};
