#pragma once

#include "Base/Types/BasicTypes.h"

#include "GameData/Geometry/Vector2D.h"
#include "GameData/Network/GameplayCommand.h"
#include "GameData/Network/NetworkEntityId.h"

namespace Network
{
	class CreatePlayerEntityCommand : public GameplayCommand
	{
	public:
		// server side
		CreatePlayerEntityCommand(Vector2D pos, ConnectionId ownerConnectionId, NetworkEntityId networkEntityId);
		// client side
		CreatePlayerEntityCommand(bool isOwner, Vector2D pos, NetworkEntityId networkEntityId);

		GameplayCommandType getType() const final { return GetType(); }
		void execute(World& world) const final;
		Ptr clone() const final;
		virtual void serverSerialize(World& world, std::vector<std::byte>& inOutStream, ConnectionId receiverConnectionId) const final;
		static Ptr ClientDeserialize(const std::vector<std::byte>& stream, size_t& inOutCursorPos);
		static GameplayCommandType GetType();

	private:
		const bool mIsOwner = false;
		const bool mIsServerSide; // can be only client or server
		const Vector2D mPos;
		const NetworkEntityId mNetworkEntityId;
		const ConnectionId mOwnerConnectionId = InvalidConnectionId;
	};
} // namespace Network
