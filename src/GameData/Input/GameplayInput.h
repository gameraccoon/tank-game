#pragma once

#include <array>

#include "GameData/Time/GameplayTimestamp.h"

namespace GameplayInput
{
	enum class KeyState
	{
		Inactive = 0,
		JustActivated = 1,
		Active = 2,
		JustDeactivated = 3,
	};

	enum class InputAxis
	{
		MoveHorizontal = 0,
		MoveVertical,
		// add new elements above this line
		Count
	};

	enum class InputKey
	{
		Shoot = 0,
		// add new elements above this line
		Count
	};

	class FrameState
	{
	public:
		struct KeyInfo
		{
			KeyState state = KeyState::Inactive;
			GameplayTimestamp lastFlipTime;

			auto operator<=>(const KeyInfo&) const noexcept = default;
		};

		using RawAxesData = std::array<float, static_cast<size_t>(InputAxis::Count)>;
		using RawKeysData = std::array<KeyInfo, static_cast<size_t>(InputKey::Count)>;

	public:
		FrameState() = default;

		template<typename RawAxes, typename RawKeys>
		FrameState(RawAxes&& rawAxesData, RawKeys&& rawKeysData)
			: mAxes(std::forward<RawAxes>(rawAxesData))
			, mKeys(std::forward<RawKeys>(rawKeysData))
		{ }

		bool operator==(const FrameState&) const noexcept = default;
		bool operator!=(const FrameState&) const noexcept = default;

		void updateAxis(InputAxis axis, float newValue);
		float getAxisValue(InputAxis axis) const;

		void updateKey(InputKey key, KeyState newState, GameplayTimestamp currentTimestamp);
		KeyState getKeyState(InputKey key) const;
		bool isKeyJustActivated(InputKey key) const;
		bool isKeyActive(InputKey key) const;
		bool isKeyJustDeactivated(InputKey key) const;
		GameplayTimestamp getLastFlipTime(InputKey key) const;

		const RawAxesData& getRawAxesData() const { return mAxes; }
		const RawKeysData& getRawKeysData() const { return mKeys; }

	private:
		RawAxesData mAxes{};
		RawKeysData mKeys;
	};
}
