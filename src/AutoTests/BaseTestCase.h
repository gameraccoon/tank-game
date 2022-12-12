#pragma once

#include "AutoTests/TestChecklist.h"

class ArgumentsParser;

class BaseTestCase
{
public:
	virtual ~BaseTestCase() = default;

	virtual TestChecklist start(const ArgumentsParser& arguments) = 0;
};
