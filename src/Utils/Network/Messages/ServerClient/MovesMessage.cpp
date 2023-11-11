#include "Base/precomp.h"

#include "Utils/Network/Messages/ServerClient/MovesMessage.h"

#include "Base/Types/Serialization.h"

#include "GameData/Components/MovementComponent.generated.h"
#include "GameData/Components/NetworkIdMappingComponent.generated.h"
#include "GameData/Components/TransformComponent.generated.h"
#include "GameData/Network/MovementHistory.h"
#include "GameData/Network/NetworkMessageIds.h"
#include "GameData/World.h"

#include "Utils/Network/CompressedInput.h"
#include "Utils/Network/GameStateRewinder.h"

namespace Network::ServerClient
{
	HAL::ConnectionManager::Message CreateMovesMessage(World& world, const TupleVector<Entity, const MovementComponent*, const TransformComponent*>& components, u32 updateIdx, GameplayTimestamp lastUpdateTimestamp, u32 lastKnownPlayerInputUpdateIdx, u32 lastKnownAllPlayersInputUpdateIdx)
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

		const NetworkIdMappingComponent* networkIdMapping = world.getWorldComponents().getOrAddComponent<const NetworkIdMappingComponent>();

		for (auto [entity, movement, transform] : components)
		{
			const GameplayTimestamp serverMoveTimestamp = movement->getUpdateTimestamp();
			// only if we moved within some agreed (between client and server) period of time
			if (serverMoveTimestamp.isInitialized() && serverMoveTimestamp.getIncreasedByUpdateCount(15) > lastUpdateTimestamp)
			{
				const auto networkIdIt = networkIdMapping->getEntityToNetworkId().find(entity);
				AssertFatal(networkIdIt != networkIdMapping->getEntityToNetworkId().end(), "We should have network id mapped for all entities that we replicate to clients");
				Serialization::AppendNumber<u64>(movesMessageData, networkIdIt->second);
				const Vector2D location = transform->getLocation();
				Serialization::AppendNumber<f32>(movesMessageData, location.x);
				Serialization::AppendNumber<f32>(movesMessageData, location.y);

				Serialization::AppendNumber<u32>(movesMessageData, serverMoveTimestamp.getRawValue());
			}
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
			GameplayTimestamp lastUpdateTimestamp(Serialization::ReadNumber<u32>(message.data, streamIndex).value_or(0));

			currentUpdateData.addMove(networkEntityId, location, lastUpdateTimestamp);
		}

		std::sort(currentUpdateData.updateHash.begin(), currentUpdateData.updateHash.end());

		gameStateRewinder.applyAuthoritativeMoves(updateIdx, hasFinalInput, std::move(currentUpdateData));
	}
} // namespace Network::ServerClient
