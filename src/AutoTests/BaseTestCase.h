#pragma once

#include "AutoTests/TestCheckList.h"

#include "raccoon-ecs/utils/system.h"

class ArgumentsParser;
class WorldHolder;

class BaseTestCase
{
public:
	virtual ~BaseTestCase() = default;

	virtual TestChecklist start(const ArgumentsParser& arguments) = 0;
};
