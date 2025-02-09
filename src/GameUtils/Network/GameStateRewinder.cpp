#include "EngineCommon/precomp.h"

#include "GameUtils/Network/GameStateRewinder.h"

#include <bitset>
#include <unordered_map>
#include <unordered_set>

#include <neargye/magic_enum.hpp>

#include "GameData/Components/NetworkOwnedEntitiesComponent.generated.h"
#include "GameData/EcsDefinitions.h"
#include "GameData/LogCategories.h"
#include "GameData/Network/EntityMoveData.h"
#include "GameData/Network/EntityMoveHash.h"
#include "GameData/WorldLayer.h"

#include "GameUtils/Network/BoundCheckedHistory.h"

class GameStateRewinder::Impl
{
public:
	/// In the history for each update we store: game state, input, moves, and commands.
	///
	/// Game state is the actual state of the game world, including all entities and their components.
	/// Game state is not communicated between client and server, it is only used for simulation.
	/// The client can resimulate game states from the past if major desyncs are detected.
	/// The server will keep the updates that it simulated untouched ((not implemented yet)but can look up the history to confirm the correctness of the clients' requests).
	///
	/// Input is the actual controller input recorded from hardware and converted into gameplay-related button states and axis values.
	/// Input is client-authoritive, however if input is lost beyond thresholds, server will predict the input and store it as authoritive.
	/// Input is sent from client to server and never back.
	///
	/// Moves is the information enough to position the entities in the world (and predict their movememnt if needed by the gameplay).
	/// Moves are server-authoritive, client will predict them, and will correct them if the server sends different data.
	/// Moves are sent from server to client and never back.
	/// Messages with moves can be lost for some updates and not resent, because the client only cares about the latest movements.
	///
	/// Commands are important information about state changes, examples can be entity creations and removals, health changes, etc.
	/// Commands are server-authoritive, client will predict them, and will rewind the game back in history to correct them and resimulate the game.
	/// Commands are sent from server to client and never back.
	/// Messages with commands are never lost, all of them are important for the client to have the same game state as the server.
	/// (not implemented yet) If the messages are lost beyond the threshold, the client should request the server to resend the full state as if it just connected.
	struct OneUpdateData
	{
		enum class SyncState : u8
		{
			// nothing is stored for this update
			NoData = 0,
			// stored predicted data
			Predicted = 1,
			// authoritive data that can't be overwritten (confirmed by the authoritive side of the connection)
			Authoritative = 2,
		};

		enum class StateType : u8
		{
			Commands = 0,
			Movement = 1,
		};

		enum class DesyncType : u8
		{
			Commands = 0,
			Movement = 1,
		};

		struct DataState
		{
			static constexpr std::array<SyncState, magic_enum::enum_count<StateType>()> EMPTY_STATE{};

			std::array<SyncState, magic_enum::enum_count<StateType>()> states{};
			std::bitset<magic_enum::enum_count<DesyncType>()> desyncedData{};
			std::unordered_set<ConnectionId> serverInputConfirmedPlayers{};
			bool hasClientInput = false;

			[[nodiscard]] SyncState getState(StateType type) const { return states[static_cast<size_t>(type)]; }
			void setState(StateType type, SyncState newState) { states[static_cast<size_t>(type)] = newState; }
			void setDesynced(DesyncType type, const bool isDesynced) { desyncedData.set(static_cast<size_t>(type), isDesynced); }
			[[nodiscard]] bool isDesynced(DesyncType type) const { return desyncedData.test(static_cast<size_t>(type)); }
			void resetDesyncedData() { desyncedData.reset(); }
		};

		struct MovementUpdateData
		{
			std::vector<EntityMoveHash> updateHash;
			std::vector<EntityMoveData> moves;
			bool onlyOwnedEntities = false;

			void reset()
			{
				updateHash.clear();
				moves.clear();
				onlyOwnedEntities = false;
			}
		};

		// flags that describe what data is stored for this update
		DataState dataState{};
		// movement produced on previous update (extracted from game state, used as checksum, not used for simulation)
		MovementUpdateData clientMovement;
		// commands generated previous update used to produce state for this update
		Network::GameplayCommandHistoryRecord gameplayCommands;
		// inputs that were used to produce game state
		GameplayInput::FrameState clientInput;
		std::unordered_map<ConnectionId, GameplayInput::FrameState> serverInput;
		// game state produced this update
		std::unique_ptr<WorldLayer> gameState;

		OneUpdateData() = default;
		~OneUpdateData() = default;
		OneUpdateData(const OneUpdateData&) = delete;
		OneUpdateData& operator=(const OneUpdateData&) = delete;
		OneUpdateData(OneUpdateData&&) = default;
		OneUpdateData& operator=(OneUpdateData&&) = default;

		bool isEmpty() const { return dataState.states == DataState::EMPTY_STATE; }
		void clear();
	};

	struct LastNonOwnedEntityMoves
	{
		std::vector<EntityMoveData> moves;
		u32 updateIdx = 0;
	};

	using History = BoundCheckedHistory<OneUpdateData, u32>;

public:
	explicit Impl(ComponentFactory& componentFactory)
	{
		OneUpdateData& updateDataZero = updateHistory.getOrCreateRecordByUpdateIdx(0);
		// set some data for the state of the 0th update to avoid special-case checks
		updateDataZero.gameState = std::make_unique<WorldLayer>(componentFactory);
	}

	OneUpdateData& getOrCreateRecordByUpdateIdx(const u32 updateIdx)
	{
		Assert(updateIdx < updateHistory.getLastStoredUpdateIdx() + Impl::DEBUG_MAX_FUTURE_UPDATES, "We are trying to append command to an update that is very far in the future. This is probably a bug. updateIndex is %u and last stored update is %u", updateIdx, updateHistory.getLastStoredUpdateIdx());
		return updateHistory.getOrCreateRecordByUpdateIdx(updateIdx);
	}

	static void SplitOutNonOwnedEntities(OneUpdateData::MovementUpdateData& ownedEntities, const OneUpdateData& updateData, const u32 updateIdx, LastNonOwnedEntityMoves& lastNonOwnedEntityMoves)
	{
		const NetworkOwnedEntitiesComponent* networkOwnedEntities = updateData.gameState->getWorldComponents().getOrAddComponent<const NetworkOwnedEntitiesComponent>();

		auto nonOwnedRange = std::ranges::partition(ownedEntities.moves, [&networkOwnedEntities](const EntityMoveData& moveData) {
			return std::ranges::find(networkOwnedEntities->getOwnedEntities(), moveData.networkEntityId) != networkOwnedEntities->getOwnedEntities().end();
		});

		if (updateIdx > lastNonOwnedEntityMoves.updateIdx)
		{
			lastNonOwnedEntityMoves.moves.clear();
			lastNonOwnedEntityMoves.moves.reserve(std::ranges::distance(nonOwnedRange));
			std::ranges::move(nonOwnedRange, std::back_inserter(lastNonOwnedEntityMoves.moves));
			lastNonOwnedEntityMoves.updateIdx = updateIdx;
		}

		ownedEntities.moves.erase(nonOwnedRange.begin(), ownedEntities.moves.end());

		Assert(ownedEntities.updateHash.empty(), "updateHash wasn't empty, it's not expected");
		for (const EntityMoveData& moveData : ownedEntities.moves)
		{
			ownedEntities.updateHash.emplace_back(moveData.networkEntityId, moveData.location, moveData.direction);
		}

		std::sort(ownedEntities.updateHash.begin(), ownedEntities.updateHash.end());

		ownedEntities.onlyOwnedEntities = true;
	}

public:
	// after reaching this number of input frames, the old input will be cleared
	constexpr static u32 MAX_INPUT_TO_PREDICT = 10;
	constexpr static u32 DEBUG_MAX_FUTURE_UPDATES = 10;

	// history of updates, may contain updates in the future
	History updateHistory;

	// moves for entities that we don't own, so we care only about latest known
	LastNonOwnedEntityMoves lastNonOwnedEntityMoves;
};

GameStateRewinder::GameStateRewinder(const HistoryType historyType, ComponentFactory& componentFactory)
	: mPimpl(std::make_unique<Impl>(componentFactory))
	, mHistoryType(historyType)
	, mNotRewindableComponents(componentFactory)
{
}

GameStateRewinder::~GameStateRewinder() = default;

u32 GameStateRewinder::getFirstStoredUpdateIdx() const
{
	return mPimpl->updateHistory.getFirstStoredUpdateIdx();
}

void GameStateRewinder::trimOldUpdates(const u32 firstUpdateToKeep)
{
	SCOPED_PROFILER("GameStateRewinder::trimOldUpdates");
	LogInfo(LOG_STATE_REWINDING, "trimOldUpdates on %s, firstUpdateToKeep=%u", mHistoryType == HistoryType::Client ? "client" : "server", firstUpdateToKeep);

	mPimpl->updateHistory.trimOldUpdates(firstUpdateToKeep, [](Impl::OneUpdateData& removedUpdateData) {
		removedUpdateData.clear();
	});
}

void GameStateRewinder::unwindBackInHistory(const u32 firstUpdateToResimulate)
{
	SCOPED_PROFILER("GameStateRewinder::unwindBackInHistory");
	LogInfo(LOG_STATE_REWINDING, "unwindBackInHistory(firstUpdateToResimulate=%u)", firstUpdateToResimulate);

	const Impl::History::ForwardRange updateDataToReset = mPimpl->updateHistory.getRecordsUnsafe(firstUpdateToResimulate, mCurrentTimeData.lastFixedUpdateIndex);
	for (const auto [updateData, updateIdx] : updateDataToReset)
	{
		updateData.dataState.resetDesyncedData();
	}

	const u32 updatesToResimulate = updateDataToReset.getUpdatesCount();
	mCurrentTimeData.lastFixedUpdateIndex -= updatesToResimulate;
	mCurrentTimeData.lastFixedUpdateTimestamp = mCurrentTimeData.lastFixedUpdateTimestamp.getDecreasedByUpdateCount(static_cast<s32>(updatesToResimulate));
}

WorldLayer& GameStateRewinder::getDynamicWorld(const u32 updateIdx) const
{
	return *mPimpl->updateHistory.getRecordUnsafe(updateIdx).gameState;
}

void GameStateRewinder::advanceSimulationToNextUpdate(const u32 newUpdateIdx)
{
	SCOPED_PROFILER("GameStateRewinder::advanceSimulationToNextUpdate");
	assertNotChangingFarFuture(newUpdateIdx);

	Impl::OneUpdateData& newUpdateData = mPimpl->getOrCreateRecordByUpdateIdx(newUpdateIdx);

	const Impl::OneUpdateData& previousUpdateData = mPimpl->updateHistory.getRecordUnsafe(newUpdateIdx - 1);

	// copy previous update data to the new update
	if (newUpdateData.gameState)
	{
		newUpdateData.gameState->overrideBy(*previousUpdateData.gameState);
	}
	else
	{
		newUpdateData.gameState = std::make_unique<WorldLayer>(*previousUpdateData.gameState);
	}
}

u32 GameStateRewinder::getLastConfirmedClientUpdateIdx() const
{
	assertClientOnly();

	const Impl::History::ReverseRange records = mPimpl->updateHistory.getAllRecordsReverse();
	for (const auto [updateData, updateIdx] : records)
	{
		const Impl::OneUpdateData::SyncState moveState = updateData.dataState.getState(Impl::OneUpdateData::StateType::Movement);
		const Impl::OneUpdateData::SyncState commandsState = updateData.dataState.getState(Impl::OneUpdateData::StateType::Commands);

		if (moveState == Impl::OneUpdateData::SyncState::Authoritative && commandsState == Impl::OneUpdateData::SyncState::Authoritative)
		{
			return updateIdx;
		}
	}

	return INVALID_UPDATE_IDX;
}

u32 GameStateRewinder::getFirstDesyncedUpdateIdx() const
{
	assertClientOnly();

	// find last non-empty update
	const Impl::History::ReverseRange allRecords = mPimpl->updateHistory.getAllRecordsReverse();
	auto lastRealUpdateIt = std::find_if(allRecords.begin(), allRecords.end(), [](const auto recordPair) {
		return !recordPair.record.isEmpty();
	});

	if (lastRealUpdateIt == allRecords.end())
	{
		return INVALID_UPDATE_IDX;
	}

	const u32 firstStoredUpdateIdx = getFirstStoredUpdateIdx();
	const u32 lastRealUpdateIdx = (*lastRealUpdateIt).updateIdx;

	const Impl::History::ForwardRange realRecords = mPimpl->updateHistory.getRecordsUnsafe(firstStoredUpdateIdx, lastRealUpdateIdx);
	for (const auto [updateData, updateIdx] : realRecords)
	{
		if (updateData.dataState.isDesynced(Impl::OneUpdateData::DesyncType::Movement) || updateData.dataState.isDesynced(Impl::OneUpdateData::DesyncType::Commands))
		{
			return updateIdx;
		}
	}

	return INVALID_UPDATE_IDX;
}

void GameStateRewinder::appendExternalCommandToHistory(const u32 updateIdx, Network::GameplayCommand::Ptr&& newCommand)
{
	assertNotChangingPast(updateIdx);
	assertNotChangingFarFuture(updateIdx);

	Impl::OneUpdateData& updateData = mPimpl->getOrCreateRecordByUpdateIdx(updateIdx);

	const Impl::OneUpdateData::SyncState commandsState = updateData.dataState.getState(Impl::OneUpdateData::StateType::Commands);
	if (commandsState == Impl::OneUpdateData::SyncState::Authoritative)
	{
		ReportError("Trying to append command to update %u that already has authoritative commands", updateIdx);
		return;
	}

	updateData.gameplayCommands.externalCommands.list.push_back(std::move(newCommand));

	updateData.dataState.setState(Impl::OneUpdateData::StateType::Commands, Impl::OneUpdateData::SyncState::Predicted);
}

void GameStateRewinder::applyAuthoritativeCommands(const u32 updateIdx, std::vector<Network::GameplayCommand::Ptr>&& commands)
{
	assertClientOnly();
	assertNotChangingFarFuture(updateIdx);

	Impl::OneUpdateData& updateData = mPimpl->getOrCreateRecordByUpdateIdx(updateIdx);

	if (updateData.dataState.getState(Impl::OneUpdateData::StateType::Commands) == Impl::OneUpdateData::SyncState::Authoritative)
	{
		ReportError("Trying to apply authoritative commands to update %u that already has final authoritative commands", updateIdx);
		return;
	}

	if (updateData.gameplayCommands.gameplayGeneratedCommands.list != commands)
	{
		LogInfo(LOG_STATE_REWINDING, "We got desynced commands for update %u", updateIdx);
		updateData.dataState.setDesynced(Impl::OneUpdateData::DesyncType::Commands, true);
	}

	updateData.gameplayCommands.gameplayGeneratedCommands.list = std::move(commands);
	updateData.dataState.setState(Impl::OneUpdateData::StateType::Commands, Impl::OneUpdateData::SyncState::Authoritative);
}

void GameStateRewinder::writeSimulatedCommands(const u32 updateIdx, const Network::GameplayCommandList& updateCommands)
{
	assertNotChangingPast(updateIdx);
	assertNotChangingFarFuture(updateIdx);

	Impl::OneUpdateData& updateData = mPimpl->getOrCreateRecordByUpdateIdx(updateIdx);

	const Impl::OneUpdateData::SyncState commandsState = updateData.dataState.getState(Impl::OneUpdateData::StateType::Commands);
	if (commandsState == Impl::OneUpdateData::SyncState::Authoritative)
	{
		return;
	}

	updateData.gameplayCommands.gameplayGeneratedCommands = updateCommands;
	updateData.dataState.setState(Impl::OneUpdateData::StateType::Commands, Impl::OneUpdateData::SyncState::Predicted);
}

bool GameStateRewinder::hasConfirmedCommandsForUpdate(const u32 updateIdx) const
{
	assertClientOnly();

	if (updateIdx >= getFirstStoredUpdateIdx() && updateIdx <= mPimpl->updateHistory.getLastStoredUpdateIdx())
	{
		const Impl::OneUpdateData& updateData = mPimpl->updateHistory.getRecordUnsafe(updateIdx);
		const Impl::OneUpdateData::SyncState state = updateData.dataState.getState(Impl::OneUpdateData::StateType::Commands);
		return state == Impl::OneUpdateData::SyncState::Authoritative;
	}

	return false;
}

const Network::GameplayCommandHistoryRecord& GameStateRewinder::getCommandsForUpdate(const u32 updateIdx) const
{
	const Impl::OneUpdateData& updateData = mPimpl->updateHistory.getRecordUnsafe(updateIdx);
	return updateData.gameplayCommands;
}

const GameplayInput::FrameState& GameStateRewinder::getPlayerInput(const ConnectionId connectionId, const u32 updateIdx) const
{
	assertServerOnly();

	static constexpr GameplayInput::FrameState emptyInput;
	const Impl::OneUpdateData& updateData = mPimpl->updateHistory.getRecordUnsafe(updateIdx);

	if (!updateData.dataState.serverInputConfirmedPlayers.contains(connectionId))
	{
		ReportError("We shouldn't call getPlayerInput to get input that wasn't confirmed, use getOrPredictPlayerInput instead");
		return emptyInput;
	}

	const auto it = updateData.serverInput.find(connectionId);
	Assert(it != updateData.serverInput.end(), "Trying to get input for player %u for update %u but there is no input for this player", connectionId, updateIdx);
	if (it == updateData.serverInput.end())
	{
		return emptyInput;
	}

	return it->second;
}

const GameplayInput::FrameState& GameStateRewinder::getOrPredictPlayerInput(const ConnectionId connectionId, const u32 updateIdx)
{
	assertServerOnly();
	assertNotChangingFarFuture(updateIdx);

	Impl::OneUpdateData& updateData = mPimpl->getOrCreateRecordByUpdateIdx(updateIdx);
	GameplayInput::FrameState& updateInput = updateData.serverInput[connectionId];
	if (updateData.dataState.serverInputConfirmedPlayers.contains(connectionId))
	{
		// we have confirmed or already predicted input for this player, so we don't need to predict anything
		return updateInput;
	}

	// we don't have any input data for this update, so we need to predict it
	const std::optional<u32> lastKnownInputUpdateIdxOption = getLastKnownInputUpdateIdxForPlayer(connectionId);
	if (!lastKnownInputUpdateIdxOption.has_value())
	{
		// we don't have any input data for this player at all, so we can't predict anything
		static constexpr GameplayInput::FrameState emptyInput;
		return emptyInput;
	}
	const u32 lastKnownInputUpdateIdx = lastKnownInputUpdateIdxOption.value();

	const u32 numUpdatesToPredict = updateIdx - lastKnownInputUpdateIdx;

	const GameplayInput::FrameState* predictedInput;
	if (numUpdatesToPredict < Impl::MAX_INPUT_TO_PREDICT)
	{
		predictedInput = &getPlayerInput(connectionId, lastKnownInputUpdateIdx);
	}
	else
	{
		// we can't predict that far, so assume player is not moving
		static constexpr GameplayInput::FrameState emptyInput;
		predictedInput = &emptyInput;
	}

	updateInput = *predictedInput;

	return updateInput;
}

void GameStateRewinder::addPlayerInput(const ConnectionId connectionId, const u32 updateIdx, const GameplayInput::FrameState& newInput)
{
	assertServerOnly();
	assertNotChangingPast(updateIdx);
	assertNotChangingFarFuture(updateIdx);

	Impl::OneUpdateData& updateData = mPimpl->getOrCreateRecordByUpdateIdx(updateIdx);
	updateData.serverInput[connectionId] = newInput;
	updateData.dataState.serverInputConfirmedPlayers.insert(connectionId);
}

std::optional<u32> GameStateRewinder::getLastKnownInputUpdateIdxForPlayer(const ConnectionId connectionId) const
{
	assertServerOnly();

	const Impl::History::ReverseRange records = mPimpl->updateHistory.getAllRecordsReverse();
	auto lastKnownInputUpdateIt = std::find_if(records.begin(), records.end(), [connectionId](const auto recordPair) {
		return recordPair.record.dataState.serverInputConfirmedPlayers.contains(connectionId);
	});

	if (lastKnownInputUpdateIt != records.end())
	{
		return (*lastKnownInputUpdateIt).updateIdx;
	}

	return std::nullopt;
}

std::optional<u32> GameStateRewinder::getLastKnownInputUpdateIdxForPlayers(const std::vector<std::pair<ConnectionId, s32>>& connections) const
{
	assertServerOnly();

	const Impl::History::ReverseRange records = mPimpl->updateHistory.getAllRecordsReverse();

	auto lastKnownInputUpdateIt = std::find_if(records.begin(), records.end(), [connections](const auto recordPair) {
		return std::all_of(connections.begin(), connections.end(), [recordPair](const auto& pair) {
			return recordPair.record.dataState.serverInputConfirmedPlayers.contains(pair.first);
		});
	});

	if (lastKnownInputUpdateIt != records.end())
	{
		return (*lastKnownInputUpdateIt).updateIdx;
	}

	return std::nullopt;
}

void GameStateRewinder::addPredictedMovementDataForUpdate(const u32 updateIdx, std::vector<EntityMoveData>&& newUpdateData)
{
	assertClientOnly();
	assertNotChangingPast(updateIdx);
	assertNotChangingFarFuture(updateIdx);

	Impl::OneUpdateData& updateData = mPimpl->getOrCreateRecordByUpdateIdx(updateIdx);

	const Impl::OneUpdateData::SyncState previousMovementDataState = updateData.dataState.getState(Impl::OneUpdateData::StateType::Movement);
	if (previousMovementDataState == Impl::OneUpdateData::SyncState::NoData || previousMovementDataState == Impl::OneUpdateData::SyncState::Predicted)
	{
		updateData.dataState.setState(Impl::OneUpdateData::StateType::Movement, Impl::OneUpdateData::SyncState::Predicted);
		updateData.clientMovement = {};
		updateData.clientMovement.moves = std::move(newUpdateData);
		for (const EntityMoveData& moveData : updateData.clientMovement.moves)
		{
			updateData.clientMovement.updateHash.emplace_back(moveData.networkEntityId, moveData.location, moveData.direction);
		}
		std::sort(updateData.clientMovement.updateHash.begin(), updateData.clientMovement.updateHash.end());
		updateData.clientMovement.onlyOwnedEntities = true;
	}
}

void GameStateRewinder::applyAuthoritativeMoves(const u32 updateIdx, std::vector<EntityMoveData>&& authoritativeMovementData)
{
	assertClientOnly();
	assertNotChangingFarFuture(updateIdx);

	const u32 firstRecordUpdateIdx = getFirstStoredUpdateIdx();

	if (updateIdx < firstRecordUpdateIdx)
	{
		// we got an update for some old state that we don't have records for, skip it
		return;
	}

	// if we already have the record
	if (updateIdx <= mCurrentTimeData.lastFixedUpdateIndex
		&& updateIdx <= mPimpl->updateHistory.getLastStoredUpdateIdx()
		&& updateIdx >= firstRecordUpdateIdx)
	{
		Impl::OneUpdateData& updateData = mPimpl->updateHistory.getRecordUnsafe(updateIdx);
		const Impl::OneUpdateData::SyncState previousMovementDataState = updateData.dataState.getState(Impl::OneUpdateData::StateType::Movement);

		// split the movement data into owned and not owned entities
		Impl::OneUpdateData::MovementUpdateData ownedEntities;
		ownedEntities.moves = std::move(authoritativeMovementData);
		Impl::SplitOutNonOwnedEntities(ownedEntities, updateData, updateIdx, mPimpl->lastNonOwnedEntityMoves);

		AssertFatal(previousMovementDataState != Impl::OneUpdateData::SyncState::Authoritative, "Got authoritative movement data for update %u that already has authoritative data", updateIdx);
		if (previousMovementDataState == Impl::OneUpdateData::SyncState::Predicted)
		{
			// we have predicted data for this update, check if it matches
			AssertFatal(updateData.clientMovement.onlyOwnedEntities, "The predicted moves were mixed with non-owned entities, this should not happen");
			if (updateData.clientMovement.updateHash != ownedEntities.updateHash)
			{
				updateData.clientMovement = std::move(ownedEntities);
				updateData.dataState.setDesynced(Impl::OneUpdateData::DesyncType::Movement, true);
				LogInfo(LOG_STATE_REWINDING, "We got desynced movement data for update %u", updateIdx);
			}
		}

		updateData.dataState.setState(Impl::OneUpdateData::StateType::Movement, Impl::OneUpdateData::SyncState::Authoritative);
	}
	else
	{
		// if the update doesn't exist, we don't yet know which entities are owned by us, so we delay the decision
		Impl::OneUpdateData& updateData = mPimpl->getOrCreateRecordByUpdateIdx(updateIdx);
		updateData.clientMovement = Impl::OneUpdateData::MovementUpdateData{};
		updateData.clientMovement.moves = std::move(authoritativeMovementData);
		updateData.dataState.setState(Impl::OneUpdateData::StateType::Movement, Impl::OneUpdateData::SyncState::Authoritative);
	}
}

const std::vector<EntityMoveData>& GameStateRewinder::getMovesForUpdate(const u32 updateIdx)
{
	assertClientOnly();

	Impl::OneUpdateData::MovementUpdateData& clientMovement = mPimpl->updateHistory.getRecordUnsafe(updateIdx).clientMovement;

	if (!clientMovement.onlyOwnedEntities)
	{
		Impl::SplitOutNonOwnedEntities(clientMovement, mPimpl->updateHistory.getRecordUnsafe(updateIdx), updateIdx, mPimpl->lastNonOwnedEntityMoves);
	}

	return clientMovement.moves;
}

bool GameStateRewinder::hasConfirmedMovesForUpdate(const u32 updateIdx) const
{
	assertClientOnly();

	if (updateIdx >= getFirstStoredUpdateIdx() && updateIdx <= mPimpl->updateHistory.getLastStoredUpdateIdx())
	{
		const Impl::OneUpdateData& updateData = mPimpl->updateHistory.getRecordUnsafe(updateIdx);
		const Impl::OneUpdateData::SyncState state = updateData.dataState.getState(Impl::OneUpdateData::StateType::Movement);
		return state == Impl::OneUpdateData::SyncState::Authoritative;
	}

	return false;
}

const std::vector<EntityMoveData>& GameStateRewinder::getLatestKnownNonOwnedEntityMoves() const
{
	return mPimpl->lastNonOwnedEntityMoves.moves;
}

std::vector<GameplayInput::FrameState> GameStateRewinder::getLastInputs(const size_t size, const u32 lastUpdateIdx) const
{
	assertClientOnly();

	std::vector<GameplayInput::FrameState> result;

	AssertFatal(lastUpdateIdx > getFirstStoredUpdateIdx(), "We are trying to get input for update (%u) that is before the first stored update (%u)", lastUpdateIdx, getFirstStoredUpdateIdx());

	const size_t inputSize = std::min(size, static_cast<size_t>(lastUpdateIdx - getFirstStoredUpdateIdx() + 1));
	result.reserve(inputSize);

	const u32 firstInputUpdate = std::max(getFirstStoredUpdateIdx(), static_cast<u32>(lastUpdateIdx + 1 - inputSize));

	const Impl::History::ForwardRange records = mPimpl->updateHistory.getRecordsUnsafe(firstInputUpdate, lastUpdateIdx);
	for (const auto [updateData, updateIdx] : records)
	{
		// first update may not have input set yet
		if (updateIdx != firstInputUpdate || hasInputForUpdate(updateIdx))
		{
			result.push_back(getInputForUpdate(updateIdx));
		}
	}

	return result;
}

bool GameStateRewinder::hasInputForUpdate(const u32 updateIdx) const
{
	assertClientOnly();

	const Impl::OneUpdateData& updateData = mPimpl->updateHistory.getRecordUnsafe(updateIdx);
	return updateData.dataState.hasClientInput;
}

const GameplayInput::FrameState& GameStateRewinder::getInputForUpdate(const u32 updateIdx) const
{
	assertClientOnly();

	const Impl::OneUpdateData& updateData = mPimpl->updateHistory.getRecordUnsafe(updateIdx);
	Assert(updateData.dataState.hasClientInput, "We are trying to get input for update (%u) that doesn't have the input set", updateIdx);
	return updateData.clientInput;
}

void GameStateRewinder::setInputForUpdate(const u32 updateIdx, const GameplayInput::FrameState& newInput)
{
	assertClientOnly();
	assertNotChangingPast(updateIdx);

	Impl::OneUpdateData& updateData = mPimpl->getOrCreateRecordByUpdateIdx(updateIdx);
	if (!updateData.dataState.hasClientInput)
	{
		updateData.clientInput = newInput;
		updateData.dataState.hasClientInput = true;
	}
#ifdef DEBUG_CHECKS
	else
	{
		if (updateData.clientInput != newInput)
		{
			ReportError("We got different input by resimulating an update, this is probably a bug");
		}
	}
#endif // DEBUG_CHECKS
}

void GameStateRewinder::setInitialClientUpdateIndex(const u32 newUpdateIndex)
{
	assertClientOnly();

	LogInfo(LOG_STATE_REWINDING, "Client sets initial update index from %u to %u (mLastStoredUpdateIdx was %u)", mCurrentTimeData.lastFixedUpdateIndex, newUpdateIndex, mPimpl->updateHistory.getLastStoredUpdateIdx());

	const s32 updateShift = static_cast<s32>(newUpdateIndex) - static_cast<s32>(mCurrentTimeData.lastFixedUpdateIndex);

	mCurrentTimeData.lastFixedUpdateTimestamp.increaseByUpdateCount(updateShift);
	mCurrentTimeData.lastFixedUpdateIndex = newUpdateIndex;
	const u32 newLastStoredUpdateIdx = static_cast<u32>(static_cast<s32>(mPimpl->updateHistory.getLastStoredUpdateIdx()) + updateShift);
	mPimpl->updateHistory.setLastStoredUpdateIdxAndCleanNegativeUpdates(newLastStoredUpdateIdx);
	mIsInitialClientUpdateIndexSet = true;
}

void GameStateRewinder::assertServerOnly() const
{
	AssertFatal(mHistoryType == HistoryType::Server, "This method should only be called on the server");
}

void GameStateRewinder::assertClientOnly() const
{
	AssertFatal(mHistoryType == HistoryType::Client, "This method should only be called on the client");
}

void GameStateRewinder::assertNotChangingPast(const u32 changedUpdateIdx) const
{
	Assert(changedUpdateIdx > mCurrentTimeData.lastFixedUpdateIndex, "We are trying to make a change to an update that is in the past. changedUpdateIdx is %u and last fixed update is %u", changedUpdateIdx, mCurrentTimeData.lastFixedUpdateIndex);
}

void GameStateRewinder::assertNotChangingFarFuture(const u32 changedUpdateIdx) const
{
	Assert(changedUpdateIdx < mCurrentTimeData.lastFixedUpdateIndex + Impl::DEBUG_MAX_FUTURE_UPDATES, "We are trying to make a change to an update that is very far in the future. This is probably a bug. changedUpdateIdx is %u and last fixed update is %u", changedUpdateIdx, mCurrentTimeData.lastFixedUpdateIndex);
}

void GameStateRewinder::Impl::OneUpdateData::clear()
{
	dataState.states = DataState::EMPTY_STATE;
	dataState.resetDesyncedData();
	dataState.serverInputConfirmedPlayers.clear();
	dataState.hasClientInput = false;
	clientMovement.reset();
	gameplayCommands.gameplayGeneratedCommands.list.clear();
	gameplayCommands.externalCommands.list.clear();
	clientInput = {};
	serverInput.clear();
}
