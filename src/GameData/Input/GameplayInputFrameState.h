#pragma once

#include <array>

#include "GameData/Input/GameplayInputConstants.h"
#include "GameData/Time/GameplayTimestamp.h"

namespace GameplayInput
{
	enum class KeyState
	{
		Inactive = 0,
		JustActivated = 1 << 0,
		JustDeactivated = 1 << 1,
		Active = 1 << 2,
	};

	KeyState MergeKeyStates(auto... state) noexcept
	{
		return static_cast<KeyState>((... | static_cast<int>(state)));
	}

	bool AreStatesIntersecting(KeyState state1, KeyState state2) noexcept;

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
		{}

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

		// for serialization and deserialization
		float getRawAxisState(size_t axisIdx) const;
		KeyInfo getRawKeyState(size_t keyIdx) const;
		void setRawAxisState(size_t axisIdx, float value);
		void setRawKeyState(size_t keyIdx, KeyState state, GameplayTimestamp lastFlipTimestamp);

	private:
		RawAxesData mAxes{};
		RawKeysData mKeys;
	};
} // namespace GameplayInput
