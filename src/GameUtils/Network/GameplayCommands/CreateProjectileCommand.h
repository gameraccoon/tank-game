#pragma once

#include "EngineData/Geometry/Vector2D.h"

#include "GameData/Enums/Direction4.generated.h"
#include "GameData/Network/GameplayCommand.h"
#include "GameData/Network/NetworkEntityId.h"

namespace Network
{
	class CreateProjectileCommand final : public GameplayCommand
	{
	public:
		CreateProjectileCommand(Vector2D pos, Direction4 direction, float speed, NetworkEntityId networkEntityId, NetworkEntityId ownerNetworkEntityId) noexcept;
		~CreateProjectileCommand() override = default;

		[[nodiscard]] GameplayCommandType getType() const final { return GetType(); }
		void execute(GameStateRewinder& gameStateRewinder, WorldLayer& world) const final;
		[[nodiscard]] Ptr clone() const final;
		void serverSerialize(WorldLayer& world, std::vector<std::byte>& inOutStream, ConnectionId receiverConnectionId) const final;
		[[nodiscard]] static Ptr ClientDeserialize(const std::vector<std::byte>& stream, size_t& inOutCursorPos);
		static GameplayCommandType GetType();

	private:
		const Vector2D mPos;
		const Direction4 mDirection;
		const float mSpeed;
		const NetworkEntityId mNetworkEntityId;
		const NetworkEntityId mOwnerNetworkEntityId;
	};
} // namespace Network
