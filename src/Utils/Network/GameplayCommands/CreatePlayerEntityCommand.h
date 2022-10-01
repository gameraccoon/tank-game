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
		enum class IsOwner
		{
			Yes,
			No,
		};

	public:
		static GameplayCommand::Ptr createServerSide(Vector2D pos, NetworkEntityId networkEntityId, ConnectionId ownerConnectionId);
		static GameplayCommand::Ptr createClientSide(Vector2D pos, NetworkEntityId networkEntityId, IsOwner isOwner);

		GameplayCommandType getType() const final { return GetType(); }
		void execute(World& world) const final;
		Ptr clone() const final;
		virtual void serverSerialize(World& world, std::vector<std::byte>& inOutStream, ConnectionId receiverConnectionId) const final;
		static Ptr ClientDeserialize(const std::vector<std::byte>& stream, size_t& inOutCursorPos);
		static GameplayCommandType GetType();

	private:
		enum class NetworkSide {
			Client,
			Server,
		};

	private:
		CreatePlayerEntityCommand(Vector2D pos, NetworkEntityId networkEntityId, IsOwner isOwner, ConnectionId ownerConnectionId, NetworkSide networkSide);

	private:
		const IsOwner mIsOwner;
		const NetworkSide mNetworkSide;
		const Vector2D mPos;
		const NetworkEntityId mNetworkEntityId;
		const ConnectionId mOwnerConnectionId;
	};
} // namespace Network
