#include "Base/precomp.h"

#include "Utils/Network/GameplayCommands/CreateProjectileCommand.h"

#include "Base/TimeConstants.h"
#include "Base/Types/Serialization.h"

#include "GameData/Components/CharacterStateComponent.generated.h"
#include "GameData/Components/ClientGameDataComponent.generated.h"
#include "GameData/Components/MovementComponent.generated.h"
#include "GameData/Components/NetworkIdComponent.generated.h"
#include "GameData/Components/NetworkIdMappingComponent.generated.h"
#include "GameData/Components/SpriteCreatorComponent.generated.h"
#include "GameData/Components/TimeLimitedLifetimeComponent.generated.h"
#include "GameData/Components/TransformComponent.generated.h"
#include "GameData/World.h"

#include "Utils/Network/GameStateRewinder.h"

#include "Utils/Network/GameplayCommands/GameplayCommandTypes.h"

namespace Network
{
	CreateProjectileCommand::CreateProjectileCommand(Vector2D pos, Vector2D direction, float speed, NetworkEntityId networkEntityId)
		: mPos(pos)
		, mDirection(direction)
		, mSpeed(speed)
		, mNetworkEntityId(networkEntityId)
	{
	}

	void CreateProjectileCommand::execute(GameStateRewinder& gameStateRewinder, World& world) const
	{
		EntityManager& worldEntityManager = world.getEntityManager();
		Entity projectileEntity = worldEntityManager.addEntity();
		{
			TransformComponent* transform = worldEntityManager.addComponent<TransformComponent>(projectileEntity);
			transform->setLocation(mPos);
			MovementComponent* movement = worldEntityManager.addComponent<MovementComponent>(projectileEntity);
			movement->setSpeed(mSpeed);
			movement->setMoveDirection(mDirection);
#ifndef DEDICATED_SERVER
			SpriteCreatorComponent* spriteCreator = worldEntityManager.addComponent<SpriteCreatorComponent>(projectileEntity);
			spriteCreator->getDescriptionsRef().emplace_back(SpriteParams{Vector2D(16, 16), ZERO_VECTOR}, "resources/textures/spawn-1.png");
#endif // !DEDICATED_SERVER
			TimeLimitedLifetimeComponent* timeLimitedLifetime = worldEntityManager.addComponent<TimeLimitedLifetimeComponent>(projectileEntity);
			timeLimitedLifetime->setDestroyTime(gameStateRewinder.getTimeData().lastFixedUpdateTimestamp.getIncreasedByUpdateCount(static_cast<s32>(1.0f / TimeConstants::ONE_FIXED_UPDATE_SEC)));
			NetworkIdComponent* networkId = worldEntityManager.addComponent<NetworkIdComponent>(projectileEntity);
			networkId->setId(mNetworkEntityId);
			NetworkIdMappingComponent* networkIdMapping = world.getWorldComponents().getOrAddComponent<NetworkIdMappingComponent>();
			networkIdMapping->getNetworkIdToEntityRef().emplace(mNetworkEntityId, projectileEntity);
			networkIdMapping->getEntityToNetworkIdRef().emplace(projectileEntity, mNetworkEntityId);
		}
	}

	GameplayCommand::Ptr CreateProjectileCommand::clone() const
	{
		return std::make_unique<CreateProjectileCommand>(*this);
	}

	void CreateProjectileCommand::serverSerialize(World& /*world*/, std::vector<std::byte>& inOutStream, ConnectionId /*receiverConnectionId*/) const
	{
		inOutStream.reserve(inOutStream.size() + 8 + 4*2 + 4*2 + 4);

		Serialization::AppendNumber<u64>(inOutStream, mNetworkEntityId);
		Serialization::AppendNumber<f32>(inOutStream, mPos.x);
		Serialization::AppendNumber<f32>(inOutStream, mPos.y);
		Serialization::AppendNumber<f32>(inOutStream, mDirection.x);
		Serialization::AppendNumber<f32>(inOutStream, mDirection.y);
		Serialization::AppendNumber<f32>(inOutStream, mSpeed);
	}

	GameplayCommand::Ptr CreateProjectileCommand::ClientDeserialize(const std::vector<std::byte>& stream, size_t& inOutCursorPos)
	{
		const NetworkEntityId serverEntityId = Serialization::ReadNumber<u64>(stream, inOutCursorPos).value_or(0);
		const float projectilePosX = Serialization::ReadNumber<f32>(stream, inOutCursorPos).value_or(0.0f);
		const float projectilePosY = Serialization::ReadNumber<f32>(stream, inOutCursorPos).value_or(0.0f);
		const float projectileDirX = Serialization::ReadNumber<f32>(stream, inOutCursorPos).value_or(0.0f);
		const float projectileDirY = Serialization::ReadNumber<f32>(stream, inOutCursorPos).value_or(0.0f);
		const float projectileSpeed = Serialization::ReadNumber<f32>(stream, inOutCursorPos).value_or(0.0f);

		return std::make_unique<CreateProjectileCommand>(Vector2D(projectilePosX, projectilePosY), Vector2D(projectileDirX, projectileDirY), projectileSpeed, serverEntityId);
	}

	GameplayCommandType CreateProjectileCommand::GetType()
	{
		return GameplayCommandType::CreateProjectile;
	}
}
