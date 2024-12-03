#include "EngineCommon/precomp.h"

#include "GameUtils/Network/GameplayCommands/CreateProjectileCommand.h"

#include "EngineCommon/TimeConstants.h"
#include "EngineCommon/Types/Serialization.h"

#include "GameData/Components/CharacterStateComponent.generated.h"
#include "GameData/Components/ClientGameDataComponent.generated.h"
#include "GameData/Components/MoveInterpolationComponent.generated.h"
#include "GameData/Components/MovementComponent.generated.h"
#include "GameData/Components/NetworkIdComponent.generated.h"
#include "GameData/Components/NetworkIdMappingComponent.generated.h"
#include "GameData/Components/ProjectileComponent.generated.h"
#include "GameData/Components/SpriteCreatorComponent.generated.h"
#include "GameData/Components/TransformComponent.generated.h"
#include "GameData/Components/WeaponComponent.generated.h"
#include "GameData/WorldLayer.h"

#include "GameUtils/Network/GameplayCommands/GameplayCommandTypes.h"
#include "GameUtils/Network/GameStateRewinder.h"

namespace Network
{
	CreateProjectileCommand::CreateProjectileCommand(Vector2D pos, Vector2D direction, float speed, NetworkEntityId networkEntityId, NetworkEntityId ownerNetworkEntityId) noexcept
		: mPos(pos)
		, mDirection(direction)
		, mSpeed(speed)
		, mNetworkEntityId(networkEntityId)
		, mOwnerNetworkEntityId(ownerNetworkEntityId)
	{
	}

	void CreateProjectileCommand::execute(GameStateRewinder& gameStateRewinder, WorldLayer& world) const
	{
		EntityManager& worldEntityManager = world.getEntityManager();
		Entity projectileEntity = worldEntityManager.addEntity();
		{
			NetworkIdMappingComponent* networkIdMapping = world.getWorldComponents().getOrAddComponent<NetworkIdMappingComponent>();
			networkIdMapping->getNetworkIdToEntityRef().emplace(mNetworkEntityId, projectileEntity);
			networkIdMapping->getEntityToNetworkIdRef().emplace(projectileEntity, mNetworkEntityId);

			TransformComponent* transform = worldEntityManager.addComponent<TransformComponent>(projectileEntity);
			transform->setLocation(mPos);

			MovementComponent* movement = worldEntityManager.addComponent<MovementComponent>(projectileEntity);
			movement->setSpeed(mSpeed);
			movement->setMoveDirection(mDirection);
#ifndef DEDICATED_SERVER
			SpriteCreatorComponent* spriteCreator = worldEntityManager.addComponent<SpriteCreatorComponent>(projectileEntity);
			spriteCreator->getDescriptionsRef().emplace_back(SpriteParams{ Vector2D(16, 16), Vector2D(0.5f, 0.5f) }, RelativeResourcePath("resources/textures/spawn-1.png"));
#endif // !DEDICATED_SERVER

			ProjectileComponent* projectile = worldEntityManager.addComponent<ProjectileComponent>(projectileEntity);
			projectile->setDestroyTime(gameStateRewinder.getTimeData().lastFixedUpdateTimestamp.getIncreasedByUpdateCount(static_cast<s32>(1.0f / TimeConstants::ONE_FIXED_UPDATE_SEC)));
			if (auto entityIt = networkIdMapping->getNetworkIdToEntity().find(mOwnerNetworkEntityId); entityIt != networkIdMapping->getNetworkIdToEntity().end())
			{
				projectile->setOwnerEntity(entityIt->second);
			}

			NetworkIdComponent* networkId = worldEntityManager.addComponent<NetworkIdComponent>(projectileEntity);
			networkId->setId(mNetworkEntityId);

			if (mOwnerNetworkEntityId != InvalidNetworkEntityId)
			{
				const auto entityIt = networkIdMapping->getNetworkIdToEntity().find(mOwnerNetworkEntityId);
				if (entityIt != networkIdMapping->getNetworkIdToEntity().end())
				{
					auto [weapon] = worldEntityManager.getEntityComponents<WeaponComponent>(entityIt->second);
					if (weapon != nullptr)
					{
						weapon->setProjectileEntity(projectileEntity);
					}
				}
			}
		}

		if (gameStateRewinder.isClient())
		{
			worldEntityManager.addComponent<MoveInterpolationComponent>(projectileEntity);
		}
	}

	GameplayCommand::Ptr CreateProjectileCommand::clone() const
	{
		return std::make_unique<CreateProjectileCommand>(*this);
	}

	void CreateProjectileCommand::serverSerialize(WorldLayer& /*world*/, std::vector<std::byte>& inOutStream, ConnectionId /*receiverConnectionId*/) const
	{
		inOutStream.reserve(inOutStream.size() + 8 + 4 * 2 + 4 * 2 + 4);

		Serialization::AppendNumber<u64>(inOutStream, mNetworkEntityId);
		Serialization::AppendNumber<f32>(inOutStream, mPos.x);
		Serialization::AppendNumber<f32>(inOutStream, mPos.y);
		Serialization::AppendNumber<f32>(inOutStream, mSpeed);
		Serialization::AppendNumber<u64>(inOutStream, mOwnerNetworkEntityId);
		Serialization::AppendNumber<f32>(inOutStream, mDirection.x);
		Serialization::AppendNumber<f32>(inOutStream, mDirection.y);
	}

	GameplayCommand::Ptr CreateProjectileCommand::ClientDeserialize(const std::vector<std::byte>& stream, size_t& inOutCursorPos)
	{
		const NetworkEntityId serverEntityId = Serialization::ReadNumber<u64>(stream, inOutCursorPos).value_or(0);
		const float projectilePosX = Serialization::ReadNumber<f32>(stream, inOutCursorPos).value_or(0.0f);
		const float projectilePosY = Serialization::ReadNumber<f32>(stream, inOutCursorPos).value_or(0.0f);
		const float projectileSpeed = Serialization::ReadNumber<f32>(stream, inOutCursorPos).value_or(0.0f);
		const NetworkEntityId ownerNetworkEntityId = Serialization::ReadNumber<u64>(stream, inOutCursorPos).value_or(0);
		const float directionX = Serialization::ReadNumber<f32>(stream, inOutCursorPos).value_or(0.0f);
		const float directionY = Serialization::ReadNumber<f32>(stream, inOutCursorPos).value_or(0.0f);

		return std::make_unique<CreateProjectileCommand>(Vector2D(projectilePosX, projectilePosY), Vector2D(directionX, directionY), projectileSpeed, serverEntityId, ownerNetworkEntityId);
	}

	GameplayCommandType CreateProjectileCommand::GetType()
	{
		return GameplayCommandType::CreateProjectile;
	}
} // namespace Network
