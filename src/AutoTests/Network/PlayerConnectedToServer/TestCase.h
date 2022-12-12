#pragma once

#include "AutoTests/BaseTestCase.h"

class ArgumentsParser;

class PlayerConnectedToServerTestCase final : public BaseTestCase
{
public:
	TestChecklist start(const ArgumentsParser& arguments) final;
};
