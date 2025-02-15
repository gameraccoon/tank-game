#include "EngineCommon/precomp.h"

#include "GameLogic/Systems/ShootingSystem.h"

#include "GameData/Components/CharacterStateComponent.generated.h"
#include "GameData/Components/GameplayCommandsComponent.generated.h"
#include "GameData/Components/NetworkEntityIdGeneratorComponent.generated.h"
#include "GameData/Components/NetworkIdMappingComponent.generated.h"
#include "GameData/Components/TransformComponent.generated.h"
#include "GameData/Components/WeaponComponent.generated.h"
#include "GameData/WorldLayer.h"

#include "GameUtils/Network/GameplayCommands/CreateProjectileCommand.h"
#include "GameUtils/SharedManagers/WorldHolder.h"

ShootingSystem::ShootingSystem(WorldHolder& worldHolder) noexcept
	: mWorldHolder(worldHolder)
{
}

void ShootingSystem::update()
{
	SCOPED_PROFILER("ShootingSystem::update");
	WorldLayer& world = mWorldHolder.getDynamicWorldLayer();

	NetworkEntityIdGeneratorComponent* networkEntityIdGenerator = world.getWorldComponents().getOrAddComponent<NetworkEntityIdGeneratorComponent>();
	GameplayCommandsComponent* gameplayCommands = world.getWorldComponents().getOrAddComponent<GameplayCommandsComponent>();

	EntityManager& entityManager = world.getEntityManager();

	NetworkIdMappingComponent* networkIdMapping = world.getWorldComponents().getOrAddComponent<NetworkIdMappingComponent>();

	entityManager.forEachComponentSetWithEntity<const WeaponComponent, const TransformComponent, const CharacterStateComponent>(
		[&gameplayCommands, networkEntityIdGenerator, &entityManager, networkIdMapping](const Entity entity, const WeaponComponent* weapon, const TransformComponent* transform, const CharacterStateComponent* characterState) {
			if (characterState->getState() == CharacterState::Shoot || characterState->getState() == CharacterState::WalkAndShoot)
			{
				if (weapon->getProjectileEntity().isValid() && entityManager.hasEntity(weapon->getProjectileEntity().getEntity()))
				{
					// we already have a projectile, don't spawn another one
					return;
				}

				const NetworkEntityId projectileNetworkId = networkEntityIdGenerator->getGeneratorRef().generateNext();
				if (const auto entityIt = networkIdMapping->getEntityToNetworkId().find(entity); entityIt != networkIdMapping->getEntityToNetworkId().end())
				{
					const NetworkEntityId ownerNetworkId = entityIt->second;
					gameplayCommands->getDataRef().list.emplace_back(std::make_unique<Network::CreateProjectileCommand>(transform->getLocation(), transform->getDirection(), 60.0f, projectileNetworkId, ownerNetworkId));
				}
			}
		}
	);
}
