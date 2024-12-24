#pragma once

#include "EngineData/Geometry/Vector2D.h"

#include "GameData/Network/GameplayCommand.h"
#include "GameData/Network/NetworkEntityId.h"

namespace Network
{
	class CreateProjectileCommand final : public GameplayCommand
	{
	public:
		CreateProjectileCommand(Vector2D pos, Vector2D direction, float speed, NetworkEntityId networkEntityId, NetworkEntityId ownerNetworkEntityId) noexcept;
		~CreateProjectileCommand() override = default;

		[[nodiscard]] GameplayCommandType getType() const override { return GetType(); }
		void execute(GameStateRewinder& gameStateRewinder, WorldHolder& worldHolder) const override;
		[[nodiscard]] Ptr clone() const override;
		void serverSerialize(WorldHolder& worldHolder, std::vector<std::byte>& inOutStream, ConnectionId receiverConnectionId) const override;
		[[nodiscard]] static Ptr ClientDeserialize(const std::vector<std::byte>& stream, size_t& inOutCursorPos);
		static GameplayCommandType GetType();

	private:
		const Vector2D mPos;
		const Vector2D mDirection;
		const float mSpeed;
		const NetworkEntityId mNetworkEntityId;
		const NetworkEntityId mOwnerNetworkEntityId;
	};
} // namespace Network
