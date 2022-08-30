#include "Base/precomp.h"

#include "Utils/Network/Messages/MovesMessage.h"

#include "Base/Types/Serialization.h"
#include "Base/Types/BasicTypes.h"

#include "GameData/Components/ClientMovesHistoryComponent.generated.h"
#include "GameData/Components/InputHistoryComponent.generated.h"
#include "GameData/Components/MovementComponent.generated.h"
#include "GameData/Components/NetworkIdMappingComponent.generated.h"
#include "GameData/Components/TimeComponent.generated.h"
#include "GameData/Components/TransformComponent.generated.h"
#include "GameData/Network/NetworkMessageIds.h"
#include "GameData/World.h"

#include "Utils/Network/CompressedInput.h"

namespace Network
{
	HAL::ConnectionManager::Message CreateMovesMessage(const TupleVector<Entity, const MovementComponent*, const TransformComponent*>& components, u32 updateIdx, GameplayTimestamp lastUpdateTimestamp, s32 indexShift, u32 lastReceivedInputUpdateIdx)
	{
		std::vector<std::byte> movesMessageData;

		const u32 clientUpdateIdx = updateIdx - indexShift;

		const bool hasMissingInput = lastReceivedInputUpdateIdx + indexShift < updateIdx;

		Serialization::AppendNumber<u8>(movesMessageData, static_cast<u8>(hasMissingInput));
		if (hasMissingInput) {
			Serialization::AppendNumber<u32>(movesMessageData, lastReceivedInputUpdateIdx);
		}

		Serialization::AppendNumber<u32>(movesMessageData, clientUpdateIdx);

		for (auto [entity, movement, transform] : components)
		{
			const GameplayTimestamp serverMoveTimestamp = movement->getUpdateTimestamp();
			// only if we moved within some agreed (between client and server) period of time
			if (serverMoveTimestamp.isInitialized() && serverMoveTimestamp.getIncreasedByUpdateCount(15) > lastUpdateTimestamp)
			{
				// Fixme should use server entity for this, need to make network ids
				Serialization::AppendNumber<u64>(movesMessageData, entity.getId());
				const Vector2D location = transform->getLocation();
				Serialization::AppendNumber<f32>(movesMessageData, location.x);
				Serialization::AppendNumber<f32>(movesMessageData, location.y);

				const GameplayTimestamp clientMoveTimestamp = serverMoveTimestamp.isInitialized() ? serverMoveTimestamp.getDecreasedByUpdateCount(indexShift) : serverMoveTimestamp;
				Serialization::AppendNumber<u32>(movesMessageData, clientMoveTimestamp.getRawValue());
			}
		}

		return HAL::ConnectionManager::Message{
			static_cast<u32>(NetworkMessageId::EntityMove),
			std::move(movesMessageData)
		};
	}

	void ApplyMovesMessage(World& world, const HAL::ConnectionManager::Message& message)
	{
		const auto [time] = world.getWorldComponents().getComponents<const TimeComponent>();
		ClientMovesHistoryComponent* clientMovesHistory = world.getNotRewindableWorldComponents().getOrAddComponent<ClientMovesHistoryComponent>();

		const u32 lastUpdateIdx = time->getValue().lastFixedUpdateIndex;

		std::vector<MovementUpdateData>& updates = clientMovesHistory->getDataRef().updates;
		const u32 lastRecordUpdateIdx = clientMovesHistory->getData().lastUpdateIdx;
		const u32 lastConfirmedUpdateIdx = clientMovesHistory->getData().lastConfirmedUpdateIdx;
		const u32 desynchedUpdateIdx = clientMovesHistory->getData().desynchedUpdateIdx;

		size_t streamIndex = HAL::ConnectionManager::Message::payloadStartPos;
		u32 lastReceivedInputUpdateIdx = 0;
		const bool hasMissingInput = (Serialization::ReadNumber<u8>(message.data, streamIndex) != 0);
		if (hasMissingInput)
		{
			lastReceivedInputUpdateIdx = Serialization::ReadNumber<u32>(message.data, streamIndex);
		}
		const u32 updateIdx = Serialization::ReadNumber<u32>(message.data, streamIndex);
		// we are not interested in values greater than the update idx of input that we are processing
		// for any non-relevant value, assume it is equal to updateIdx
		lastReceivedInputUpdateIdx = hasMissingInput ? std::min(lastReceivedInputUpdateIdx, updateIdx) : updateIdx;

		AssertFatal(lastReceivedInputUpdateIdx <= updateIdx, "We can't have input update from the future");

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

		const u32 firstRecordUpdateIdx = static_cast<u32>(lastRecordUpdateIdx + 1 - updates.size());

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

		InputHistoryComponent* inputHistory = world.getNotRewindableWorldComponents().getOrAddComponent<InputHistoryComponent>();

		const size_t firstKnowwnInputUpdateIndex = inputHistory->getLastInputUpdateIdx() + 1 - inputHistory->getInputs().size();
		const size_t updatedRecordIdx = updateIdx - firstRecordUpdateIdx;

		if (updateIdx < firstKnowwnInputUpdateIndex)
		{
			// we didn't record input for this frame yet, so no need to correct it
			return;
		}

		AssertFatal(updatedRecordIdx < updates.size(), "Index for movements history is out of bounds");
		MovementUpdateData& currentUpdateData = updates[updatedRecordIdx];

		std::vector<EntityMoveHash> oldMovesData = std::move(currentUpdateData.updateHash);

		NetworkIdMappingComponent* networkIdMapping = world.getWorldComponents().getOrAddComponent<NetworkIdMappingComponent>();

		const size_t dataSize = message.data.size();
		while (streamIndex < dataSize)
		{
			const u64 serverEntityId = Serialization::ReadNumber<u64>(message.data, streamIndex);
			const auto entityIt = networkIdMapping->getNetworkIdToEntity().find(serverEntityId);
			Assert(entityIt != networkIdMapping->getNetworkIdToEntity().end(), "Server entity is not found on the client");
			Entity entity = (entityIt != networkIdMapping->getNetworkIdToEntity().end()) ? entityIt->second : Entity(0);
			Vector2D location;
			location.x = Serialization::ReadNumber<f32>(message.data, streamIndex);
			location.y = Serialization::ReadNumber<f32>(message.data, streamIndex);
			GameplayTimestamp lastUpdateTimestamp(Serialization::ReadNumber<u32>(message.data, streamIndex));

			currentUpdateData.addMove(entity, location, lastUpdateTimestamp);
		}

		std::sort(currentUpdateData.updateHash.begin(), currentUpdateData.updateHash.end());

		bool areMovesDesynched = false;
		if (oldMovesData != currentUpdateData.updateHash)
		{
			areMovesDesynched = true;

			if (hasMissingInput)
			{
				const std::vector<GameplayInput::FrameState>& inputs = inputHistory->getInputs();
				// check if the server was able to correctly predict our input for that frame
				// and if it was able, then mark record as desynched, since then our prediction was definitely incorrect
				if (inputHistory->getLastInputUpdateIdx() >= updateIdx && (inputHistory->getLastInputUpdateIdx() + 1 - inputs.size() <= lastReceivedInputUpdateIdx))
				{
					const size_t indexShift = inputHistory->getLastInputUpdateIdx() + 1 - inputs.size();
					for (u32 i = updateIdx; i > lastReceivedInputUpdateIdx; --i)
					{
						if (inputs[i - 1 - indexShift] != inputs[i - indexShift])
						{
							// server couldn't predict our input correctly, we can't trust its movement prediction either
							// mark as accepted to keep our local version until we get moves with confirmed input
							areMovesDesynched = false;
							break;
						}
					}
				}
				else
				{
					ReportFatalError("We lost some input records that we need to confirm correctness of input on the server (%u, %u, %u, %u)", updateIdx, lastReceivedInputUpdateIdx, inputs.size(), inputHistory->getLastInputUpdateIdx());
				}
			}
		}

		if (areMovesDesynched)
		{
			clientMovesHistory->getDataRef().desynchedUpdateIdx = updateIdx;
		}
		else
		{
			clientMovesHistory->getDataRef().lastConfirmedUpdateIdx = updateIdx;
		}

		// trim inputs to the last received by server input, updates before it don't interest us anymore
		const size_t updatesCountAfterTrim = inputHistory->getLastInputUpdateIdx() - lastReceivedInputUpdateIdx + 1;
		std::vector<GameplayInput::FrameState>& inputs = inputHistory->getInputsRef();
		inputs.erase(inputs.begin(), inputs.begin() + (inputs.size() - std::min(updatesCountAfterTrim, inputs.size())));
	}
}
