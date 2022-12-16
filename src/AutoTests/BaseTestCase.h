#pragma once

#include "AutoTests/TestChecks.h"

#include "raccoon-ecs/system.h"

class ArgumentsParser;
class WorldHolder;

class BaseTestCase
{
public:
	virtual ~BaseTestCase() = default;

	virtual TestChecklist start(const ArgumentsParser& arguments) = 0;
};
