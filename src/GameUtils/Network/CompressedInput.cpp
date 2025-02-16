#include "EngineCommon/precomp.h"

#include "GameUtils/Network/CompressedInput.h"

#include <algorithm>
#include <array>
#include <vector>

#include "EngineCommon/Types/BasicTypes.h"
#include "EngineCommon/Types/Serialization.h"

#include "GameData/Input/InputHistory.h"

namespace Utils
{
	struct KeyInputChange
	{
		GameplayInput::KeyState state;
		GameplayTimestamp lastFlipTime;
		size_t index;
	};

	struct OneKeyHistory
	{
		size_t count = 0;
		std::array<KeyInputChange, Input::MAX_INPUT_HISTORY_SEND_SIZE> changes;
	};

	void AppendInputHistory(std::vector<std::byte>& stream, const std::vector<GameplayInput::FrameState>& inputs, size_t inputsToSend)
	{
		const size_t offset = inputs.size() - inputsToSend;

		// collect indexes of axes that have non-zero values
		using AxisInputChangeSet = std::array<bool, static_cast<size_t>(GameplayInput::InputAxis::Count)>;
		AxisInputChangeSet axisInputChangesSet;
		axisInputChangesSet.fill(false);
		for (size_t i = 0; i < inputsToSend; ++i)
		{
			const GameplayInput::FrameState& input = inputs[offset + i];

			for (size_t axisIndex = 0; axisIndex < static_cast<size_t>(GameplayInput::InputAxis::Count); ++axisIndex)
			{
				axisInputChangesSet[axisIndex] |= (input.getRawAxisState(axisIndex) != 0.0f);
			}
		}

		// collect changes to keys
		using KeyInputHistory = std::array<OneKeyHistory, static_cast<size_t>(GameplayInput::InputKey::Count)>;
		KeyInputHistory keyInputChanges;
		for (size_t i = 0; i < inputsToSend; ++i)
		{
			const GameplayInput::FrameState& input = inputs[offset + i];

			for (size_t keyIndex = 0; keyIndex < static_cast<size_t>(GameplayInput::InputKey::Count); ++keyIndex)
			{
				GameplayInput::FrameState::KeyInfo keyInfo = input.getRawKeyState(keyIndex);
				OneKeyHistory& oneKeyHistory = keyInputChanges[keyIndex];

				if (i == 0
					|| oneKeyHistory.changes[oneKeyHistory.count - 1].state != keyInfo.state
					|| oneKeyHistory.changes[oneKeyHistory.count - 1].lastFlipTime != keyInfo.lastFlipTime)
				{
					oneKeyHistory.changes[oneKeyHistory.count].state = keyInfo.state;
					oneKeyHistory.changes[oneKeyHistory.count].lastFlipTime = keyInfo.lastFlipTime;
					oneKeyHistory.changes[oneKeyHistory.count].index = i;
					++oneKeyHistory.count;
				}
			}
		}

		const size_t nonZeroAxesCount = std::count(axisInputChangesSet.begin(), axisInputChangesSet.end(), true);

		static_assert(static_cast<size_t>(GameplayInput::InputAxis::Count) <= 256, "u8 is too small to represent amount of axes");
		Serialization::AppendNumberNarrowCast<u8>(stream, nonZeroAxesCount);

		// send full input history for every axis that has at least one non-zero frame
		for (size_t axisIndex = 0; axisIndex < static_cast<size_t>(GameplayInput::InputAxis::Count); ++axisIndex)
		{
			if (axisInputChangesSet[axisIndex])
			{
				static_assert(static_cast<size_t>(GameplayInput::InputAxis::Count) <= 256, "u8 is too small to represent amount of axes");
				Serialization::AppendNumberNarrowCast<u8>(stream, axisIndex);

				for (size_t i = 0; i < inputsToSend; ++i)
				{
					const GameplayInput::FrameState& input = inputs[offset + i];
					Serialization::AppendNumber<f32>(stream, input.getRawAxisState(axisIndex));
				}
			}
		}

		// write delta-compressed key states
		for (size_t keyIndex = 0; keyIndex < static_cast<size_t>(GameplayInput::InputKey::Count); ++keyIndex)
		{
			OneKeyHistory& oneKeyHistory = keyInputChanges[keyIndex];
			AssertFatal(oneKeyHistory.count > 0 && oneKeyHistory.changes[0].index == 0, "We should always have the first frame filled for each key");

			for (size_t changeIdx = 0; changeIdx < oneKeyHistory.count; ++changeIdx)
			{
				const KeyInputChange& keyChange = oneKeyHistory.changes[changeIdx];
				const size_t endFrameIdx = (changeIdx < oneKeyHistory.count - 1) ? oneKeyHistory.changes[changeIdx + 1].index : inputsToSend;

				static_assert(Input::MAX_INPUT_HISTORY_SEND_SIZE <= 256, "u8 is too small to fit history length");
				Serialization::AppendNumberNarrowCast<u8>(stream, endFrameIdx);
				static_assert(static_cast<size_t>(GameplayInput::InputKey::Count) <= 256, "u8 is too small to represent amount of keys");
				Serialization::AppendNumber<u8>(stream, static_cast<u8>(keyChange.state));
				Serialization::AppendNumber<u32>(stream, keyChange.lastFlipTime.getRawValue());
			}
		}
	}

	std::vector<GameplayInput::FrameState> ReadInputHistory(const std::vector<std::byte>& stream, size_t inputsToRead, size_t& streamIndex)
	{
		std::vector<GameplayInput::FrameState> result;
		result.resize(inputsToRead);

		const size_t changedAxesCount = Serialization::ReadNumber<u8>(stream, streamIndex).value_or(0);

		// fill non-zero axes, zero values should be filled by default
		for (size_t i = 0; i < changedAxesCount; ++i)
		{
			const size_t axisIndex = Serialization::ReadNumber<u8>(stream, streamIndex).value_or(0);
			for (size_t frameIdx = 0; frameIdx < inputsToRead; ++frameIdx)
			{
				const float value = Serialization::ReadNumber<f32>(stream, streamIndex).value_or(0.0f);
				result[frameIdx].setRawAxisState(axisIndex, value);
			}
		}

		// fill keys data
		for (size_t keyIndex = 0; keyIndex < static_cast<size_t>(GameplayInput::InputKey::Count); ++keyIndex)
		{
			size_t nextFrameToProcess = 0;
			while (nextFrameToProcess < inputsToRead)
			{
				const size_t frameEndIdx = Serialization::ReadNumber<u8>(stream, streamIndex).value_or(0);
				const GameplayInput::KeyState state = static_cast<GameplayInput::KeyState>(Serialization::ReadNumber<u8>(stream, streamIndex).value_or(0));
				const GameplayTimestamp lastFlipTimestamp{ Serialization::ReadNumber<u32>(stream, streamIndex).value_or(0) };

				for (; nextFrameToProcess < frameEndIdx; ++nextFrameToProcess)
				{
					result[nextFrameToProcess].setRawKeyState(keyIndex, state, lastFlipTimestamp);
				}
				AssertFatal(nextFrameToProcess <= inputsToRead, "Writing out of bounds");
			}
		}

		return result;
	}
} // namespace Utils
