#include "Base/precomp.h"

#include "Utils/Network/CompressedInput.h"

#include "GameData/Components/InputHistoryComponent.generated.h"

#include "Base/Types/BasicTypes.h"
#include "Base/Types/Serialization.h"

namespace Utils
{
	void WriteInputHistory(std::vector<std::byte>& stream, const std::vector<GameplayInput::FrameState>& inputs, size_t inputsToSend)
	{
		const size_t offset = inputs.size() - inputsToSend;
		for (size_t i = 0; i < inputsToSend; ++i)
		{
			const GameplayInput::FrameState& input = inputs[offset + i];

			for (float axisValue : input.getRawAxesData())
			{
				Serialization::WriteNumber<f32>(stream, axisValue);
			}

			for (GameplayInput::FrameState::KeyInfo keyInfo : input.getRawKeysData())
			{
				Serialization::WriteNumber<s8>(stream, static_cast<s8>(keyInfo.state));
				Serialization::WriteNumber<u32>(stream, keyInfo.lastFlipTime.getRawValue());
			}
		}
	}

	std::vector<GameplayInput::FrameState> ReadInputHistory(const std::vector<std::byte>& stream, size_t inputsToRead, size_t& streamIndex)
	{
		std::vector<GameplayInput::FrameState> result;
		result.resize(inputsToRead);

		GameplayInput::FrameState::RawAxesData axesData;
		GameplayInput::FrameState::RawKeysData keysData;

		for (size_t idx = 0; idx < inputsToRead; ++idx)
		{
			for (float& axisValue : axesData)
			{
				axisValue = Serialization::ReadNumber<f32>(stream, streamIndex);
			}

			for (GameplayInput::FrameState::KeyInfo& keyInfo : keysData)
			{
				keyInfo.state = static_cast<GameplayInput::KeyState>(Serialization::ReadNumber<s8>(stream, streamIndex));
				keyInfo.lastFlipTime = GameplayTimestamp(Serialization::ReadNumber<u32>(stream, streamIndex));
			}

			result[idx] = GameplayInput::FrameState(axesData, keysData);
		}

		return result;
	}
}
