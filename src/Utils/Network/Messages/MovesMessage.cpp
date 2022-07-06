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
	HAL::ConnectionManager::Message CreateMovesMessage(const TupleVector<Entity, const MovementComponent*, const TransformComponent*>& components, u32 updateIdx)
	{
		std::vector<std::byte> movesMessageData;

		Serialization::WriteNumber<u32>(movesMessageData, updateIdx);

		for (auto [entity, movement, transform] : components)
		{
			// only if we moved this or last frame
			if (movement->getNextStep() == ZERO_VECTOR && movement->getPreviousStep() == ZERO_VECTOR)
			{
				// Fixme should use server entity for this, need to make network ids
				Serialization::WriteNumber<u64>(movesMessageData, entity.getId());
				const Vector2D location = transform->getLocation();
				Serialization::WriteNumber<s32>(movesMessageData, static_cast<s32>(location.x));
				Serialization::WriteNumber<s32>(movesMessageData, static_cast<s32>(location.y));
				const Vector2D nextStep = movement->getNextStep();
				Serialization::WriteNumber<s32>(movesMessageData, static_cast<s32>(nextStep.x));
				Serialization::WriteNumber<s32>(movesMessageData, static_cast<s32>(nextStep.y));
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
		const u32 firstConfirmedIdx = clientMovesHistory->getData().firstConfirmedUpdateIdx;
		const u32 desynchedUpdateIdx = clientMovesHistory->getData().desynchedUpdate;

		size_t streamIndex = 0;
		const u32 updateIdx = Serialization::ReadNumber<u32>(message.data, streamIndex);

		if (updateIdx > lastUpdateIdx)
		{
			// something went very wrong, we should have confirmed moves from the future
			ReportError("Correction from the future, possible desynchronized time");
			return;
		}

		AssertFatal(lastUpdateIdx == lastRecordUpdateIdx, "We should always have input record for the last frame");

		const u32 firstRecordUpdateIdx = lastRecordUpdateIdx + 1 - updates.size();

		if (updateIdx < firstRecordUpdateIdx)
		{
			// we got an update for some old state that we don't have records for, skip it
			return;
		}

		if (updateIdx <= firstConfirmedIdx)
		{
			Assert(updateIdx != firstConfirmedIdx, "Processing snapshot that has already been processed, this shouldn't happen");
			// we have snapshots later than this that are already confirmed, no need to do anything
			return;
		}

		if (desynchedUpdateIdx != std::numeric_limits<u32>::max() && updateIdx <= desynchedUpdateIdx)
		{
			Assert(updateIdx != desynchedUpdateIdx, "Processing snapshot that has already been processed, this shouldn't happen");
			// we have snapshots later than this that are confirmed to be desynchronized, no need to do anything
			return;
		}

		const size_t updatedRecordIdx = updateIdx - firstRecordUpdateIdx;

		AssertFatal(updatedRecordIdx < updates.size(), "Index for movements history is out of bounds");
		MovementUpdateData& currentUpdateData = updates[updatedRecordIdx];

		std::vector<EntityMoveData> oldMovesData = std::move(currentUpdateData.moves);

		const size_t dataSize = message.data.size();
		while (streamIndex < dataSize)
		{
			Entity entity(Serialization::ReadNumber<u64>(message.data, streamIndex));
			IntVector2D location;
			location.x = Serialization::ReadNumber<s32>(message.data, streamIndex);
			location.y = Serialization::ReadNumber<s32>(message.data, streamIndex);
			IntVector2D nextMovement;
			nextMovement.x = Serialization::ReadNumber<s32>(message.data, streamIndex);
			nextMovement.y = Serialization::ReadNumber<s32>(message.data, streamIndex);

			currentUpdateData.moves.emplace_back(entity, location, nextMovement);
		}

		std::sort(
			currentUpdateData.moves.begin(),
			currentUpdateData.moves.end(),
			[](const EntityMoveData& l, const EntityMoveData& r)
			{
				return l.entity < r.entity;
			}
		);

		if (oldMovesData == currentUpdateData.moves)
		{
			clientMovesHistory->getDataRef().firstConfirmedUpdateIdx = updateIdx;
		}
		else
		{
			clientMovesHistory->getDataRef().desynchedUpdate = updateIdx;
		}
	}
}
