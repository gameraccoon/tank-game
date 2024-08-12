#include "EngineCommon/precomp.h"

#include "GameUtils/Network/Messages/ServerClient/MovesMessage.h"

#include "EngineCommon/Types/Serialization.h"

#include "GameData/Components/NetworkIdComponent.generated.h"
#include "GameData/Components/TransformComponent.generated.h"
#include "GameData/Network/MovementHistory.h"
#include "GameData/Network/NetworkMessageIds.h"

#include "GameUtils/Network/FrameTimeCorrector.h"
#include "GameUtils/Network/GameStateRewinder.h"

namespace Network::ServerClient
{
	template<typename T, typename... Values>
	static T packBoolsIntoBitset(Values... values)
	{
		static_assert(sizeof...(Values) <= sizeof(T) * 8, "Too many values to pack into bitset");
		T result = 0;
		size_t bitIndex = 0;
		((result |= (values << bitIndex++)), ...);
		return result;
	}

	template<typename T, size_t bitIndex>
	static bool isBitSet(T bitset)
	{
		static_assert(bitIndex < sizeof(T) * 8, "Bit index is out of range");
		return (bitset & (1 << bitIndex)) != 0;
	}

	HAL::Network::Message CreateMovesMessage(const TupleVector<const TransformComponent*, const NetworkIdComponent*>& components, const u32 updateIdx, const u32 lastKnownPlayerInputUpdateIdx, const s32 indexShift)
	{
		std::vector<std::byte> movesMessageData;

		const bool hasMissingInput = lastKnownPlayerInputUpdateIdx < updateIdx;
		const bool hasIndexShift = indexShift != 0;

		Serialization::AppendNumber<u8>(movesMessageData, packBoolsIntoBitset<u8>(hasMissingInput, hasIndexShift));
		if (hasMissingInput)
		{
			Serialization::AppendNumber<u32>(movesMessageData, lastKnownPlayerInputUpdateIdx);
		}

		if (hasIndexShift)
		{
			Serialization::AppendNumber<s32>(movesMessageData, indexShift);
		}

		Serialization::AppendNumber<u32>(movesMessageData, updateIdx);

		for (auto [transform, networkId] : components)
		{
			Serialization::AppendNumber<u64>(movesMessageData, networkId->getId());
			const Vector2D location = transform->getLocation();
			const Vector2D direction = transform->getDirection();
			Serialization::AppendNumber<f32>(movesMessageData, location.x);
			Serialization::AppendNumber<f32>(movesMessageData, location.y);
			Serialization::AppendNumber<f32>(movesMessageData, direction.x);
			Serialization::AppendNumber<f32>(movesMessageData, direction.y);
		}

		return HAL::Network::Message{
			static_cast<u32>(NetworkMessageId::EntityMove),
			movesMessageData
		};
	}

	void ApplyMovesMessage(GameStateRewinder& gameStateRewinder, FrameTimeCorrector& frameTimeCorrector, const HAL::Network::Message& message)
	{
		size_t streamIndex = HAL::Network::Message::payloadStartPos;
		u32 lastReceivedInputUpdateIdx = 0;
		const u8 bitset = Serialization::ReadNumber<u8>(message.data, streamIndex).value_or(0);
		const bool hasMissingInput = isBitSet<u8, 0>(bitset);
		const bool hasIndexShift = isBitSet<u8, 1>(bitset);

		if (hasMissingInput)
		{
			lastReceivedInputUpdateIdx = Serialization::ReadNumber<u32>(message.data, streamIndex).value_or(0);
		}

		s32 indexShift = 0;
		if (hasIndexShift)
		{
			indexShift = Serialization::ReadNumber<s32>(message.data, streamIndex).value_or(0);
			LogInfo("Index shift requested: %d", indexShift);
		}
		frameTimeCorrector.updateIndexShift(indexShift);

		const u32 updateIdx = Serialization::ReadNumber<u32>(message.data, streamIndex).value_or(0);
		// we are not interested in values greater than the update idx of input that we are processing
		// for any non-relevant value, assume it is equal to updateIdx
		lastReceivedInputUpdateIdx = hasMissingInput ? std::min(lastReceivedInputUpdateIdx, updateIdx) : updateIdx;

		AssertFatal(lastReceivedInputUpdateIdx <= updateIdx, "We can't have input update from the future");

		MovementUpdateData currentUpdateData;
		currentUpdateData.moves.reserve((message.data.size() - streamIndex) / (8 + 4 + 4 + 4));
		const size_t dataSize = message.data.size();
		while (streamIndex < dataSize)
		{
			const u64 networkEntityId = Serialization::ReadNumber<u64>(message.data, streamIndex).value_or(0);
			Vector2D location{};
			location.x = Serialization::ReadNumber<f32>(message.data, streamIndex).value_or(0);
			location.y = Serialization::ReadNumber<f32>(message.data, streamIndex).value_or(0);
			Vector2D direction{};
			direction.x = Serialization::ReadNumber<f32>(message.data, streamIndex).value_or(0);
			direction.y = Serialization::ReadNumber<f32>(message.data, streamIndex).value_or(0);

			currentUpdateData.addMove(networkEntityId, location, direction);
		}

		std::sort(currentUpdateData.updateHash.begin(), currentUpdateData.updateHash.end());

		gameStateRewinder.applyAuthoritativeMoves(updateIdx, std::move(currentUpdateData));
	}
} // namespace Network::ServerClient
