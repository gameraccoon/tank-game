#pragma once

#include <map>
#include <memory>
#include <string>

class TestCheck
{
public:
	virtual ~TestCheck() = default;
	[[nodiscard]] virtual bool isPassed() const = 0;
	[[nodiscard]] virtual std::string describe() const = 0;
};

class BasicTestCheck final : public TestCheck
{
public:
	bool mIsPassed = false;
	std::string mDescription;

	BasicTestCheck(const BasicTestCheck&) = delete;
	BasicTestCheck(BasicTestCheck&&) = delete;

	template<typename T>
	explicit BasicTestCheck(T&& description)
		: mDescription(std::forward<T>(description))
	{}

	[[nodiscard]] bool isPassed() const final { return mIsPassed; }
	[[nodiscard]] std::string describe() const final { return mDescription; }
};

struct TestChecklist
{
	using Checks = std::map<std::string, std::unique_ptr<TestCheck>>;

	TestChecklist() = default;
	TestChecklist(const TestChecklist&) = default;
	TestChecklist(TestChecklist&&) = default;

	template<typename T, typename = std::enable_if_t<std::is_same_v<std::decay_t<T>, Checks>>>
	explicit TestChecklist(T&& checks)
		: checks(std::forward<T>(checks))
	{}

	TestChecklist(std::string&& key, std::unique_ptr<TestCheck> check)
	{
		checks.emplace(std::move(key), std::move(check));
	}

	Checks checks;
};
