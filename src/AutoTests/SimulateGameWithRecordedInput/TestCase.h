#pragma once

#include "AutoTests/BaseTestCase.h"

class ArgumentsParser;

class SimulateGameWithRecordedInputTestCase final : public BaseTestCase
{
public:
	SimulateGameWithRecordedInputTestCase(const char* inputFilePath, int maxFramesCount);

	TestChecklist start(const ArgumentsParser& arguments) final;

private:
	const char* mInputFilePath;
	int mFramesLeft;
};
