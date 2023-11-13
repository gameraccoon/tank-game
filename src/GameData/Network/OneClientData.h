#pragma once

#include "GameData/EcsDefinitions.h"

struct OneClientData
{
	OptionalEntity playerEntity;
	// difference between expected and actual index of last received input
	s32 indexShift = 0;
};
