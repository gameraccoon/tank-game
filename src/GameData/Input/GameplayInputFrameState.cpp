#include "EngineCommon/precomp.h"

#include "GameData/Input/GameplayInputFrameState.h"

namespace GameplayInput
{
	void FrameState::updateAxis(InputAxis axis, const float newValue)
	{
		mAxes[static_cast<size_t>(axis)] = newValue;
	}

	float FrameState::getAxisValue(InputAxis axis) const
	{
		return mAxes[static_cast<size_t>(axis)];
	}

	void FrameState::updateKey(InputKey key, const KeyState newState, const GameplayTimestamp currentTimestamp)
	{
		KeyInfo& info = mKeys[static_cast<size_t>(key)];

		if (!info.lastFlipTime.isInitialized()) [[unlikely]]
		{
			info.lastFlipTime = currentTimestamp;
		}

		if (AreStatesIntersecting(info.state, KeyState::Active) != AreStatesIntersecting(newState, KeyState::Active))
		{
			info.lastFlipTime = currentTimestamp;
		}

		info.state = newState;
	}

	KeyState FrameState::getKeyState(const InputKey key) const
	{
		return mKeys[static_cast<size_t>(key)].state;
	}

	bool FrameState::isKeyJustActivated(const InputKey key) const
	{
		return AreStatesIntersecting(getKeyState(key), KeyState::JustActivated);
	}

	bool FrameState::isKeyActive(const InputKey key) const
	{
		return AreStatesIntersecting(getKeyState(key), KeyState::Active);
	}

	bool FrameState::isKeyJustDeactivated(const InputKey key) const
	{
		return AreStatesIntersecting(getKeyState(key), KeyState::JustDeactivated);
	}

	GameplayTimestamp FrameState::getLastFlipTime(InputKey key) const
	{
		return mKeys[static_cast<size_t>(key)].lastFlipTime;
	}

	float FrameState::getRawAxisState(const size_t axisIdx) const
	{
		return mAxes[axisIdx];
	}

	FrameState::KeyInfo FrameState::getRawKeyState(const size_t keyIdx) const
	{
		return mKeys[keyIdx];
	}

	void FrameState::setRawAxisState(const size_t axisIdx, const float value)
	{
		mAxes[axisIdx] = value;
	}

	void FrameState::setRawKeyState(const size_t keyIdx, const KeyState state, const GameplayTimestamp lastFlipTimestamp)
	{
		mKeys[keyIdx] = { state, lastFlipTimestamp };
	}
} // namespace GameplayInput

static_assert(std::is_trivially_copyable<GameplayInput::FrameState>(), "GameplayInput should be trivially copyable");
