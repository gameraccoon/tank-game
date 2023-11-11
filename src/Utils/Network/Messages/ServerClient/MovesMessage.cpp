#include "Base/precomp.h"

#include "Utils/Network/Messages/ServerClient/MovesMessage.h"

#include "Base/Types/Serialization.h"

#include "GameData/Components/NetworkIdComponent.generated.h"
#include "GameData/Components/TransformComponent.generated.h"
#include "GameData/Network/MovementHistory.h"
#include "GameData/Network/NetworkMessageIds.h"

#include "Utils/Network/CompressedInput.h"
#include "Utils/Network/GameStateRewinder.h"

namespace Network::ServerClient
{
	HAL::ConnectionManager::Message CreateMovesMessage(const TupleVector<const TransformComponent*, const NetworkIdComponent*>& components, u32 updateIdx, u32 lastKnownPlayerInputUpdateIdx, u32 lastKnownAllPlayersInputUpdateIdx)
	{
		std::vector<std::byte> movesMessageData;

		const bool hasMissingInput = lastKnownPlayerInputUpdateIdx < updateIdx;
		const bool hasFinalInputs = lastKnownAllPlayersInputUpdateIdx >= updateIdx;

		Serialization::AppendNumber<u8>(movesMessageData, static_cast<u8>(static_cast<u8>(hasMissingInput) + (static_cast<u8>(hasFinalInputs) << 1)));
		if (hasMissingInput)
		{
			Serialization::AppendNumber<u32>(movesMessageData, lastKnownPlayerInputUpdateIdx);
		}

		Serialization::AppendNumber<u32>(movesMessageData, updateIdx);

		for (auto [transform, networkId] : components)
		{
			Serialization::AppendNumber<u64>(movesMessageData, networkId->getId());
			const Vector2D location = transform->getLocation();
			Serialization::AppendNumber<f32>(movesMessageData, location.x);
			Serialization::AppendNumber<f32>(movesMessageData, location.y);
		}

		return HAL::ConnectionManager::Message{
			static_cast<u32>(NetworkMessageId::EntityMove),
			movesMessageData
		};
	}

	void ApplyMovesMessage(GameStateRewinder& gameStateRewinder, const HAL::ConnectionManager::Message& message)
	{
		size_t streamIndex = HAL::ConnectionManager::Message::payloadStartPos;
		u32 lastReceivedInputUpdateIdx = 0;
		const u8 bitset = Serialization::ReadNumber<u8>(message.data, streamIndex).value_or(0);
		const bool hasMissingInput = (bitset & 1) != 0;
		const bool hasFinalInput = (bitset & (1 << 1)) != 0;
		if (hasMissingInput)
		{
			lastReceivedInputUpdateIdx = Serialization::ReadNumber<u32>(message.data, streamIndex).value_or(0);
		}
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

			currentUpdateData.addMove(networkEntityId, location);
		}

		std::sort(currentUpdateData.updateHash.begin(), currentUpdateData.updateHash.end());

		gameStateRewinder.applyAuthoritativeMoves(updateIdx, hasFinalInput, std::move(currentUpdateData));
	}
} // namespace Network::ServerClient
