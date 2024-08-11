#pragma once

#include "raccoon-ecs/utils/system.h"

#include "AutoTests/TestCheckList.h"
class ArgumentsParser;

class BaseTestCase
{
public:
	virtual ~BaseTestCase() = default;

	virtual TestChecklist start(const ArgumentsParser& arguments) = 0;
};
