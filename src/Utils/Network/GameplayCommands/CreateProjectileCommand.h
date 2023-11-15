#pragma once

#include "Base/Types/BasicTypes.h"

#include "GameData/Geometry/Vector2D.h"
#include "GameData/Network/GameplayCommand.h"
#include "GameData/Network/NetworkEntityId.h"

namespace Network
{
	class CreateProjectileCommand final : public GameplayCommand
	{
	public:
		CreateProjectileCommand(Vector2D pos, Vector2D direction, float speed, NetworkEntityId networkEntityId);
		~CreateProjectileCommand() = default;

		[[nodiscard]] GameplayCommandType getType() const final { return GetType(); }
		void execute(GameStateRewinder& gameStateRewinder, World& world) const final;
		[[nodiscard]] Ptr clone() const final;
		void serverSerialize(World& world, std::vector<std::byte>& inOutStream, ConnectionId receiverConnectionId) const final;
		[[nodiscard]] static Ptr ClientDeserialize(const std::vector<std::byte>& stream, size_t& inOutCursorPos);
		static GameplayCommandType GetType();

	private:
		const Vector2D mPos;
		const Vector2D mDirection;
		const float mSpeed;
		const NetworkEntityId mNetworkEntityId;
	};
} // namespace Network
