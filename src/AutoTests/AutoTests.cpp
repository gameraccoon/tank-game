#include "EngineCommon/precomp.h"

#include "AutoTests/AutoTests.h"

#include <iostream>

#include <raccoon-ecs/error_handling.h>

#include "EngineCommon/Random/Random.h"

#include "GameData/LogCategories.h"

#include "EngineUtils/Application/ArgumentsParser.h"

#include "AutoTests/BaseTestCase.h"
#include "AutoTests/Network/PlayerConnectedToServer/TestCase.h"
#include "AutoTests/Network/TwoPlayersSeeEachOther/TestCase.h"
#include "AutoTests/SimulateGameWithRecordedInput/TestCase.h"
#include "AutoTests/TestCheckList.h"

namespace AutoTests
{
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

	static bool ValidateChecklist(const TestChecklist& checklist)
	{
		size_t failedChecksCount = 0;
		for (const auto& check : checklist.getChecks())
		{
			if (!check->hasPassed())
			{
				if (check->hasBeenValidated())
				{
					LogInfo(LOG_AUTOTESTS, "Test check failed: %s", check->getErrorMessage());
				}
				else
				{
					LogInfo(LOG_AUTOTESTS, "Test check was not validated, assume failed: %s", check->getErrorMessage());
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
			LogInfo(LOG_AUTOTESTS, "Failed %u checks out of %u", failedChecksCount, totalChecksCount);
			return false;
		}
		else
		{
			LogInfo(LOG_AUTOTESTS, "Passed %d checks out of %d", totalChecksCount, totalChecksCount);
			return true;
		}
	}

	bool RunTests(const ArgumentsParser& arguments)
	{
		// limit values of seed, to be able to pass any of the possible seeds through command line
		int seed = std::random_device()() % static_cast<unsigned int>(std::numeric_limits<int>::max());
		if (arguments.hasArgument("randseed"))
		{
			const auto seedValue = arguments.getIntArgumentValue("randseed");
			if (seedValue.hasValue())
			{
				seed = static_cast<unsigned int>(seedValue.getValue());
			}
			else
			{
				std::cout << seedValue.getError() << "\n";
				return false;
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
			return true;
		}

		if (!arguments.hasArgument("case"))
		{
			std::cout << "Test case name has not been provided, use:\n\t--list to get the list of available test cases\n\t--case <name> to run a specific test case\n";
			return false;
		}

		const auto caseIt = cases.find(arguments.getArgumentValue("case").value_or(""));
		if (caseIt == cases.end())
		{
			std::cout << "Unknown test '" << arguments.getArgumentValue("case").value_or("") << "'\n";
			return false;
		}

		LogInit(LOG_AUTOTESTS, "Random seed is %u", seed);

		const std::unique_ptr<BaseTestCase> testCase = caseIt->second();
		const TestChecklist checklist = testCase->start(arguments);
		const bool isSuccessful = ValidateChecklist(checklist);

		std::cout << "Test run " << (isSuccessful ? "was successful" : "failed, see the full log for errors") << "\n";
		return isSuccessful;
	}
} // namespace AutoTests
