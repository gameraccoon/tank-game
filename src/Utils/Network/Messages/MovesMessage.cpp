#include "Base/precomp.h"

#include "Utils/Network/Messages/MovesMessage.h"

#include "Base/Types/Serialization.h"
#include "Base/Types/BasicTypes.h"

#include "GameData/Components/ClientMovesHistoryComponent.generated.h"
#include "GameData/Components/MovementComponent.generated.h"
#include "GameData/Components/TimeComponent.generated.h"
#include "GameData/Components/TransformComponent.generated.h"
#include "GameData/Network/NetworkMessageIds.h"
#include "GameData/World.h"

#include "Utils/Network/CompressedInput.h"

namespace Network
{
	HAL::ConnectionManager::Message CreateMovesMessage(const TupleVector<Entity, const MovementComponent*, const TransformComponent*>& components, u32 updateIdx, GameplayTimestamp lastUpdateTimestamp, s32 indexShift)
	{
		std::vector<std::byte> movesMessageData;

		const u32 clientUpdateIdx = updateIdx - indexShift;

		Serialization::WriteNumber<u32>(movesMessageData, clientUpdateIdx);

		for (auto [entity, movement, transform] : components)
		{
			const GameplayTimestamp serverMoveTimestamp = movement->getUpdateTimestamp();
			// only if we moved within some agreed (between client and server) period of time
			if (serverMoveTimestamp.isInitialized() && serverMoveTimestamp.getIncreasedByUpdateCount(15) > lastUpdateTimestamp)
			{
				// Fixme should use server entity for this, need to make network ids
				Serialization::WriteNumber<u64>(movesMessageData, entity.getId());
				const Vector2D location = transform->getLocation();
				Serialization::WriteNumber<f32>(movesMessageData, location.x);
				Serialization::WriteNumber<f32>(movesMessageData, location.y);

				const GameplayTimestamp clientMoveTimestamp = serverMoveTimestamp.isInitialized() ? serverMoveTimestamp.getDecreasedByUpdateCount(indexShift) : serverMoveTimestamp;
				Serialization::WriteNumber<u32>(movesMessageData, clientMoveTimestamp.getRawValue());
			}
		}

		return HAL::ConnectionManager::Message{
			static_cast<u32>(NetworkMessageId::EntityMove),
			std::move(movesMessageData)
		};
	}

	void ApplyMovesMessage(World& world, HAL::ConnectionManager::Message&& message)
	{
		const auto [time] = world.getWorldComponents().getComponents<const TimeComponent>();
		ClientMovesHistoryComponent* clientMovesHistory = world.getNotRewindableWorldComponents().getOrAddComponent<ClientMovesHistoryComponent>();

		const u32 lastUpdateIdx = time->getValue().lastFixedUpdateIndex;

		std::vector<MovementUpdateData>& updates = clientMovesHistory->getDataRef().updates;
		const u32 lastRecordUpdateIdx = clientMovesHistory->getData().lastUpdateIdx;
		const u32 lastConfirmedUpdateIdx = clientMovesHistory->getData().lastConfirmedUpdateIdx;
		const u32 desynchedUpdateIdx = clientMovesHistory->getData().desynchedUpdateIdx;

		size_t streamIndex = 0;
		const u32 updateIdx = Serialization::ReadNumber<u32>(message.data, streamIndex);

		if (updateIdx > lastUpdateIdx)
		{
			// during debugging we can have situations when server thread tempararely went ahead with simulating the game
			// skip this data until client thread catches up or server thread corrects its update index shift
			// if it visually lags because of this return then some of these doesn't work correctly:
			// - fixed dt loop with ability to catch up after a lag (and ability to recover after staying on a breakpoint)
			// - server ability to correct update index shift for clients due to changed RTT
			return;
		}

		AssertFatal(lastUpdateIdx == lastRecordUpdateIdx, "We should always have input record for the last frame");

		const u32 firstRecordUpdateIdx = lastRecordUpdateIdx + 1 - updates.size();

		if (updateIdx < firstRecordUpdateIdx)
		{
			// we got an update for some old state that we don't have records for, skip it
			return;
		}

		if (updateIdx <= lastConfirmedUpdateIdx)
		{
			// we have snapshots later than this that are already confirmed, no need to do anything
			return;
		}

		if (desynchedUpdateIdx != std::numeric_limits<u32>::max() && updateIdx <= desynchedUpdateIdx)
		{
			// we have snapshots later than this that are confirmed to be desynchronized, no need to do anything
			return;
		}

		const size_t updatedRecordIdx = updateIdx - firstRecordUpdateIdx;

		AssertFatal(updatedRecordIdx < updates.size(), "Index for movements history is out of bounds");
		MovementUpdateData& currentUpdateData = updates[updatedRecordIdx];

		std::vector<EntityMoveHash> oldMovesData = std::move(currentUpdateData.updateHash);
		std::vector<EntityMoveData> oldMovesDataTest = std::move(currentUpdateData.moves);

		const size_t dataSize = message.data.size();
		while (streamIndex < dataSize)
		{
			Entity entity(Serialization::ReadNumber<u64>(message.data, streamIndex));
			Vector2D location;
			location.x = Serialization::ReadNumber<f32>(message.data, streamIndex);
			location.y = Serialization::ReadNumber<f32>(message.data, streamIndex);
			GameplayTimestamp lastUpdateTimestamp(Serialization::ReadNumber<u32>(message.data, streamIndex));

			currentUpdateData.addMove(entity, location, lastUpdateTimestamp);
		}

		std::sort(currentUpdateData.updateHash.begin(), currentUpdateData.updateHash.end());

		if (oldMovesData == currentUpdateData.updateHash)
		{
			clientMovesHistory->getDataRef().lastConfirmedUpdateIdx = updateIdx;
		}
		else
		{
			clientMovesHistory->getDataRef().desynchedUpdateIdx = updateIdx;
		}
	}
}
