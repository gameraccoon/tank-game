#pragma once

#include <limits>
#include <vector>

#include "Base/Types/BasicTypes.h"

#include "GameData/Geometry/Vector2D.h"
#include "GameData/Network/NetworkEntityId.h"
#include "GameData/Time/GameplayTimestamp.h"

struct EntityMoveHash
{
	EntityMoveHash(NetworkEntityId networkEntityId, Vector2D location)
		: entityHash(networkEntityId)
		, locationHashX(static_cast<s32>(location.x))
		, locationHashY(static_cast<s32>(location.y))
	{}

	NetworkEntityId entityHash;
	s32 locationHashX;
	s32 locationHashY;

	bool operator==(const EntityMoveHash& other) const noexcept = default;
	bool operator<(const EntityMoveHash& other) const noexcept { return entityHash < other.entityHash; };
};

struct EntityMoveData
{
	NetworkEntityId networkEntityId;
	Vector2D location;
	GameplayTimestamp timestamp;
};

struct MovementUpdateData
{
	std::vector<EntityMoveHash> updateHash;
	std::vector<EntityMoveData> moves;

	void addMove(NetworkEntityId networkEntityId, Vector2D location, GameplayTimestamp timestamp)
	{
		AssertFatal(updateHash.size() == moves.size(), "Vector sizes mismatch in moves history, this should never happen");
		updateHash.emplace_back(networkEntityId, location);
		moves.emplace_back(networkEntityId, location, timestamp);
	}

	void addHash(NetworkEntityId networkEntityId, Vector2D location)
	{
		updateHash.emplace_back(networkEntityId, location);
		AssertFatal(moves.empty(), "We should add hashes only to history records that doesn't contain real moves");
	}
};

struct MovementHistory
{
	// we use indexes of updates when moves were created

	std::vector<MovementUpdateData> updates;
	u32 lastUpdateIdx = 0;
	u32 lastConfirmedUpdateIdx = 0;
	u32 updateIdxProducedDesyncedMoves = std::numeric_limits<u32>::max();
};
