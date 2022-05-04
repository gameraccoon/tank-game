#pragma once

#include <numeric>
#include <nlohmann/json_fwd.hpp>

#include "Base/Types/BasicTypes.h"

class GameplayTimestamp
{
public:
	// as 1/30000 of a second
	using TimeValueType = u64;

public:
	constexpr GameplayTimestamp() = default;
	explicit constexpr GameplayTimestamp(TimeValueType time) noexcept : mTimestamp(time) {}

	[[nodiscard]] bool isInitialized() const noexcept;

	auto operator<=>(const GameplayTimestamp& other) const noexcept = default;

	void increaseByFloatTime(float passedTime) noexcept;
	[[nodiscard]] GameplayTimestamp getIncreasedByFloatTime(float passedTime) const noexcept;

	friend void to_json(nlohmann::json& outJson, const GameplayTimestamp timestamp);
	friend void from_json(const nlohmann::json& json, GameplayTimestamp& outTimestamp);

	TimeValueType getRawValue() const { return mTimestamp; }

public:
	static constexpr float TimeMultiplier = 300.0f;

private:
	static constexpr TimeValueType UNINITIALIZED_TIME = std::numeric_limits<TimeValueType>::max();
	TimeValueType mTimestamp = UNINITIALIZED_TIME;
};
