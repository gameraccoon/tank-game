#include "EngineCommon/precomp.h"

#include "GameUtils/Network/GameplayCommands/CreateProjectileCommand.h"

#include "EngineCommon/TimeConstants.h"
#include "EngineCommon/Types/Serialization.h"

#include "GameData/Components/ClientGameDataComponent.generated.h"
#include "GameData/Components/MoveInterpolationComponent.generated.h"
#include "GameData/Components/MovementComponent.generated.h"
#include "GameData/Components/NetworkIdComponent.generated.h"
#include "GameData/Components/NetworkIdMappingComponent.generated.h"
#include "GameData/Components/NetworkOwnedEntitiesComponent.generated.h"
#include "GameData/Components/ProjectileComponent.generated.h"
#include "GameData/Components/SpriteCreatorComponent.generated.h"
#include "GameData/Components/TransformComponent.generated.h"
#include "GameData/Components/WeaponComponent.generated.h"
#include "GameData/WorldLayer.h"

#include "GameUtils/Network/GameplayCommands/GameplayCommandTypes.h"
#include "GameUtils/Network/GameStateRewinder.h"

namespace Network
{
	CreateProjectileCommand::CreateProjectileCommand(const Vector2D pos, const Vector2D direction, const float speed, const NetworkEntityId networkEntityId, const NetworkEntityId ownerNetworkEntityId) noexcept
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

		NetworkOwnedEntitiesComponent* networkOwnedEntities = world.getWorldComponents().getOrAddComponent<NetworkOwnedEntitiesComponent>();
		const bool isOwningClient = gameStateRewinder.isClient()
			&& mOwnerNetworkEntityId != InvalidNetworkEntityId
			&& std::ranges::find(networkOwnedEntities->getOwnedEntities(), mOwnerNetworkEntityId) != networkOwnedEntities->getOwnedEntities().end();

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
			if (const auto entityIt = networkIdMapping->getNetworkIdToEntity().find(mOwnerNetworkEntityId); entityIt != networkIdMapping->getNetworkIdToEntity().end())
			{
				projectile->setOwnerEntity(entityIt->second);
			}

			NetworkIdComponent* networkId = worldEntityManager.addComponent<NetworkIdComponent>(projectileEntity);
			networkId->setId(mNetworkEntityId);

			if (mOwnerNetworkEntityId != InvalidNetworkEntityId)
			{
				const auto playerEntityIt = networkIdMapping->getNetworkIdToEntity().find(mOwnerNetworkEntityId);
				if (playerEntityIt != networkIdMapping->getNetworkIdToEntity().end())
				{
					auto [weapon] = worldEntityManager.getEntityComponents<WeaponComponent>(playerEntityIt->second);
					if (weapon != nullptr)
					{
						weapon->setProjectileEntity(projectileEntity);
					}
				}

				// if we own the owner, we also own the projectile
				if (isOwningClient)
				{
					networkOwnedEntities->getOwnedEntitiesRef().push_back(mNetworkEntityId);
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

	GameplayCommand::Ptr CreateProjectileCommand::ClientDeserialize(const std::span<const std::byte> stream, size_t& inOutCursorPos)
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
