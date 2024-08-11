#include "EngineCommon/precomp.h"

#include <iostream>

#include <raccoon-ecs/error_handling.h>

#include "EngineCommon/Random/Random.h"

#include "EngineUtils/Application/ArgumentsParser.h"

#include "AutoTests/BaseTestCase.h"
#include "AutoTests/Network/PlayerConnectedToServer/TestCase.h"
#include "AutoTests/Network/TwoPlayersSeeEachOther/TestCase.h"
#include "AutoTests/SimulateGameWithRecordedInput/TestCase.h"
#include "AutoTests/TestCheckList.h"

using CasesMap = std::map<std::string, std::function<std::unique_ptr<BaseTestCase>()>>;

static CasesMap GetCases()
{
	CasesMap cases;

	cases.emplace("PlayerConnectedToServer", []() {
		return std::make_unique<PlayerConnectedToServerTestCase>();
	});
	cases.emplace("Shoot", []() {
		return std::make_unique<SimulateGameWithRecordedInputTestCase>("resources/autotests/shoot/input", 1, 220);
	});
	cases.emplace("Shoot2Players", []() {
		return std::make_unique<SimulateGameWithRecordedInputTestCase>("resources/autotests/shoot/input", 2, 220);
	});
	cases.emplace("TwoPlayersSeeEachOther", []() {
		return std::make_unique<TwoPlayersSeeEachOtherTestCase>();
	});

	return cases;
}

bool ValidateChecklist(const TestChecklist& checklist)
{
	size_t failedChecksCount = 0;
	for (const auto& check : checklist.getChecks())
	{
		if (!check->hasPassed())
		{
			if (check->hasBeenValidated())
			{
				LogInfo("Test check failed: %s", check->getErrorMessage());
			}
			else
			{
				LogInfo("Test check was not validated, assume failed: %s", check->getErrorMessage());
			}
			++failedChecksCount;
		}
		else
		{
			Assert(check->hasBeenValidated(), "Test check has passed but was not validated. This looks like a logical error in the test code.");
		}
	}

	const size_t totalChecksCount = checklist.getChecks().size();

	if (failedChecksCount > 0)
	{
		LogInfo("Failed %u checks out of %u", failedChecksCount, totalChecksCount);
		return false;
	}
	else
	{
		LogInfo("Passed %d checks out of %d", totalChecksCount, totalChecksCount);
		return true;
	}
}

int main(int argc, char** argv)
{
	INITIALIZE_STRING_IDS();

	ArgumentsParser arguments(argc, argv);

	unsigned int seed = std::random_device()();
	if (arguments.hasArgument("randseed"))
	{
		auto seedValue = arguments.getIntArgumentValue("randseed");
		if (seedValue.hasValue())
		{
			seed = static_cast<unsigned int>(seedValue.getValue());
		}
		else
		{
			std::cout << seedValue.getError() << "\n";
			return 1;
		}
	}

	Random::gGlobalGenerator = std::mt19937(seed);

#ifdef RACCOON_ECS_DEBUG_CHECKS_ENABLED
	RaccoonEcs::gErrorHandler = [](const std::string& error) { ReportFatalError(error); };
#endif // RACCOON_ECS_DEBUG_CHECKS_ENABLED

	gGlobalAssertHandler = [] { std::terminate(); };

	auto cases = GetCases();

	if (arguments.hasArgument("list"))
	{
		for (const auto& casePair : cases)
		{
			std::cout << casePair.first << "\n";
		}
		return 0;
	}

	if (!arguments.hasArgument("case"))
	{
		std::cout << "Test case name has not been provided, use:\n\t--list to get the list of available test cases\n\t--case <name> to run a specific test case\n";
		return 1;
	}

	auto caseIt = cases.find(arguments.getArgumentValue("case").value());
	if (caseIt != cases.end())
	{
		LogInit("Random seed is %u", seed);

		std::unique_ptr<BaseTestCase> testCase = caseIt->second();
		TestChecklist checklist = testCase->start(arguments);
		const bool isSuccessful = ValidateChecklist(checklist);

		std::cout << "Test run " << (isSuccessful ? "was successful" : "failed, see the full log for errors") << "\n";
		return isSuccessful ? 0 : 1;
	}
	else
	{
		std::cout << "Unknown test '" << arguments.getArgumentValue("case").value() << "'\n";
		return 1;
	}
}
