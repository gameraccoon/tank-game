#pragma once

#include "EngineCommon/Types/BasicTypes.h"

#include "EngineData/Geometry/Vector2D.h"

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
		~CreatePlayerEntityCommand() override = default;

		[[nodiscard]] static Ptr createServerSide(Vector2D pos, NetworkEntityId networkEntityId, ConnectionId ownerConnectionId);
		[[nodiscard]] static Ptr createClientSide(Vector2D pos, NetworkEntityId networkEntityId, IsOwner isOwner);

		[[nodiscard]] GameplayCommandType getType() const override { return GetType(); }
		void execute(GameStateRewinder& gameStateRewinder, WorldLayer& world) const override;
		[[nodiscard]] Ptr clone() const override;
		void serverSerialize(WorldLayer& world, std::vector<std::byte>& inOutStream, ConnectionId receiverConnectionId) const override;
		[[nodiscard]] static Ptr ClientDeserialize(const std::vector<std::byte>& stream, size_t& inOutCursorPos);
		static GameplayCommandType GetType();

	private:
		CreatePlayerEntityCommand(Vector2D pos, NetworkEntityId networkEntityId, IsOwner isOwner, ConnectionId ownerConnectionId);

	private:
		const IsOwner mIsOwner;
		const Vector2D mPos;
		const NetworkEntityId mNetworkEntityId;
		const ConnectionId mOwnerConnectionId;
	};
} // namespace Network
