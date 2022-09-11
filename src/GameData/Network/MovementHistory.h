#pragma once

#include <limits>
#include <vector>

#include "Base/Types/BasicTypes.h"

#include "GameData/EcsDefinitions.h"
#include "GameData/Geometry/Vector2D.h"
#include "GameData/Time/GameplayTimestamp.h"

struct EntityMoveHash
{
	EntityMoveHash(Entity entity, Vector2D location)
		: entityHash(entity.getId())
		, locationHashX(static_cast<s32>(location.x))
		, locationHashY(static_cast<s32>(location.y))
	{}

	Entity::EntityId entityHash;
	s32 locationHashX;
	s32 locationHashY;

	bool operator==(const EntityMoveHash& other) const noexcept = default;
	bool operator<(const EntityMoveHash& other) const noexcept { return entityHash < other.entityHash; };
};

struct EntityMoveData
{
	Entity entity;
	Vector2D location;
	GameplayTimestamp timestamp;
};

struct MovementUpdateData
{
	std::vector<EntityMoveHash> updateHash;
	std::vector<EntityMoveData> moves;

	void addMove(Entity entity, Vector2D location, GameplayTimestamp timestamp)
	{
		AssertFatal(updateHash.size() == moves.size(), "Vector sizes mismatch in moves history, this should never happen");
		updateHash.emplace_back(entity, location);
		moves.emplace_back(entity, location, timestamp);
	}

	void addHash(Entity entity, Vector2D location)
	{
		updateHash.emplace_back(entity, location);
		AssertFatal(moves.empty(), "We should add hashes only to history records that doen't contain real moves");
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
