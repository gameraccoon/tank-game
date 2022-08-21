#include "Base/precomp.h"

#include "GameData/Input/GameplayInput.h"

namespace GameplayInput
{
	void FrameState::updateAxis(InputAxis axis, float newValue)
	{
		mAxes[static_cast<size_t>(axis)] = newValue;
	}

	float FrameState::getAxisValue(InputAxis axis) const
	{
		return mAxes[static_cast<size_t>(axis)];
	}

	void FrameState::updateKey(InputKey key, KeyState newState, GameplayTimestamp currentTimestamp)
	{
		KeyInfo& info = mKeys[static_cast<size_t>(key)];
		if ((info.state == KeyState::Active || info.state == KeyState::JustActivated) != (newState == KeyState::Active || newState == KeyState::JustActivated)
			|| !info.lastFlipTime.isInitialized())
		{
			info.lastFlipTime = currentTimestamp;
		}
		info.state = newState;
	}

	KeyState FrameState::getKeyState(InputKey key) const
	{
		return mKeys[static_cast<size_t>(key)].state;
	}

	bool FrameState::isKeyJustActivated(InputKey key) const
	{
		return getKeyState(key) == KeyState::JustActivated;
	}

	bool FrameState::isKeyActive(InputKey key) const
	{
		const KeyState state = getKeyState(key);
		return state == KeyState::Active || state == KeyState::JustActivated;
	}

	bool FrameState::isKeyJustDeactivated(InputKey key) const
	{
		return getKeyState(key) == KeyState::JustDeactivated;
	}

	GameplayTimestamp FrameState::getLastFlipTime(InputKey key) const
	{
		return mKeys[static_cast<size_t>(key)].lastFlipTime;
	}

	float FrameState::getRawAxisState(size_t axisIdx) const
	{
		return mAxes[axisIdx];
	}

	FrameState::KeyInfo FrameState::getRawKeyState(size_t keyIdx) const
	{
		return mKeys[keyIdx];
	}

	void FrameState::setRawAxisState(size_t axisIdx, float value)
	{
		mAxes[axisIdx] = value;
	}

	void FrameState::setRawKeyState(size_t keyIdx, KeyState state, GameplayTimestamp lastFlipTimestamp)
	{
		mKeys[keyIdx] = {state, lastFlipTimestamp};
	}
}

static_assert(std::is_trivially_copyable<GameplayInput::FrameState>(), "GameplayInput should be trivially copyable");
