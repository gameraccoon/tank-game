#include "Base/precomp.h"

#include <iostream>

#include "Base/Random/Random.h"

#include <raccoon-ecs/error_handling.h>

#include "Utils/Application/ArgumentsParser.h"

#include "AutoTests/BaseTestCase.h"
#include "AutoTests/TestChecklist.h"
#include "AutoTests/Network/PlayerConnectedToServer/TestCase.h"

using CasesMap = std::map<std::string, std::function<std::unique_ptr<BaseTestCase>()>>;

static CasesMap GetCases()
{
	return CasesMap
	({
		{
			"PlayerConnectedToServer",
			[](){
				return std::make_unique<PlayerConnectedToServerTestCase>();
			}
		}
	});
}

bool ValidateChecklist(const TestChecklist& checklist)
{
	bool result = true;
	for(const auto& checkPair : checklist.checks)
	{
		if (!checkPair.second->isPassed())
		{
			LogInfo("Test check failed: %s. %s", checkPair.first, checkPair.second->describe());
			result = false;
			continue;
		}
	}
	return result;
}

int main(int argc, char** argv)
{
	ArgumentsParser arguments(argc, argv);

	unsigned int seed = std::random_device()();
	if (arguments.hasArgument("randseed"))
	{
		std::string seedStr = arguments.getArgumentValue("randseed");
		seed = static_cast<unsigned int>(std::atoi(seedStr.c_str()));
	}

	Random::gGlobalGenerator = std::mt19937(seed);

#ifdef RACCOON_ECS_DEBUG_CHECKS_ENABLED
	RaccoonEcs::gErrorHandler = [](const std::string& error) { ReportFatalError(error); };
#endif // RACCOON_ECS_DEBUG_CHECKS_ENABLED

	gGlobalAssertHandler = []{ std::terminate(); };

	auto cases = GetCases();

	if (arguments.hasArgument("list-cases"))
	{
		for (const auto& casePair : cases)
		{
			std::cout << casePair.first << "\n";
		}
		return 0;
	}

	if (!arguments.hasArgument("case"))
	{
		LogError("Test case name has not been provided");
		return 1;
	}

	auto caseIt = cases.find(arguments.getArgumentValue("case"));
	if (caseIt != cases.end())
	{
		LogInit("Random seed is %u", seed);

		std::unique_ptr<BaseTestCase> testCase = caseIt->second();
		TestChecklist checklist = testCase->start(arguments);
		const bool isSuccessful = ValidateChecklist(checklist);
		if (isSuccessful)
		{
			LogInfo("Test case %s has been passed", arguments.getArgumentValue("case"));
		}
		else
		{
			LogError("Test case %s has been failed", arguments.getArgumentValue("case"));
		}
		return isSuccessful ? 0 : 1;
	}
	else
	{
		LogError("Unknown test " + arguments.getArgumentValue("case"));
		return 1;
	}
}
