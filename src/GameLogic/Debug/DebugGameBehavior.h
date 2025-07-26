#pragma once

#include "GameLogic/Debug/DebugRecordedInput.h"
#include "GameLogic/Debug/DebugRecordedNetwork.h"

class Game;
class ArgumentsParser;

class DebugGameBehavior
{
public:
	explicit DebugGameBehavior(int instanceIndex);

	void preInnerUpdate(Game& game);
	void postInnerUpdate(Game& game);
	void processArguments(const ArgumentsParser& arguments);

private:
	DebugRecordedInput mDebugRecordedInput;
	DebugRecordedNetwork mDebugRecordedNetwork;

	int mFramesBeforeShutdown = -1;
};
