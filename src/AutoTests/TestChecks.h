#pragma once

#include <memory>
#include <string>
#include <vector>

class TestCheck
{
public:
	virtual ~TestCheck() = default;
	[[nodiscard]] virtual bool isChecked() const = 0;
	[[nodiscard]] virtual bool hasPassed() const = 0;
	[[nodiscard]] virtual std::string getErrorMessage() const = 0;
};

class SimpleTestCheck final : public TestCheck
{
public:
	SimpleTestCheck(const SimpleTestCheck&) = delete;
	SimpleTestCheck(SimpleTestCheck&&) = delete;

	template<typename Str, typename = std::enable_if_t<std::is_convertible_v<Str, std::string>>>
	explicit SimpleTestCheck(Str&& errorMessage)
		: mErrorMessage(std::forward<Str>(errorMessage))
	{}

	[[nodiscard]] bool hasPassed() const final { return mIsPassed; }
	[[nodiscard]] std::string getErrorMessage() const final { return mErrorMessage; }

	void checkAsPassed()
	{
		mIsPassed = true;
		mIsChecked = true;
	}

	void checkAsFailed()
	{
		mIsPassed = false;
		mIsChecked = true;
	}

	[[nodiscard]] bool isChecked() const override { return mIsChecked; }

private:
	bool mIsPassed = false;
	bool mIsChecked = false;
	std::string mErrorMessage;
};

struct TestChecklist
{
	std::vector<std::unique_ptr<TestCheck>> checks;

	template<typename Str, typename = std::enable_if_t<std::is_convertible_v<Str, std::string>>>
	SimpleTestCheck& addSimpleCheck(Str&& errorMessage)
	{
		checks.emplace_back(std::make_unique<SimpleTestCheck>(std::forward<Str>(errorMessage)));
		return static_cast<SimpleTestCheck&>(*checks.back());
	}

	TestCheck& addCheck(std::unique_ptr<TestCheck>&& check)
	{
		checks.emplace_back(std::move(check));
		return *checks.back();
	}

	template<typename Check, typename... Args>
	TestCheck& emplaceCheck(Args&&... args)
	{
		checks.emplace_back(std::make_unique<Check>(std::forward<Args>(args)...));
		return *checks.back();
	}

	[[nodiscard]] bool areAllChecked() const
	{
		for (const auto& check : checks)
		{
			if (!check->isChecked())
			{
				return false;
			}
		}
		return true;
	}
};
