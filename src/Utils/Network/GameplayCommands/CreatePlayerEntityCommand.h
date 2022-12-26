#pragma once

#include "Base/Types/BasicTypes.h"

#include "GameData/Geometry/Vector2D.h"
#include "GameData/Network/GameplayCommand.h"
#include "GameData/Network/NetworkEntityId.h"

namespace Network
{
	class CreatePlayerEntityCommand final : public GameplayCommand
	{
	public:
		enum class IsOwner
		{
			Yes,
			No,
		};

	public:
		[[nodiscard]] static GameplayCommand::Ptr createServerSide(Vector2D pos, NetworkEntityId networkEntityId, ConnectionId ownerConnectionId);
		[[nodiscard]] static GameplayCommand::Ptr createClientSide(Vector2D pos, NetworkEntityId networkEntityId, IsOwner isOwner);

		[[nodiscard]] GameplayCommandType getType() const final { return GetType(); }
		void execute(GameStateRewinder& gameStateRewinder, World& world) const final;
		[[nodiscard]] Ptr clone() const final;
		void serverSerialize(World& world, std::vector<std::byte>& inOutStream, ConnectionId receiverConnectionId) const final;
		[[nodiscard]] static Ptr ClientDeserialize(const std::vector<std::byte>& stream, size_t& inOutCursorPos);
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
