#include "Base/precomp.h"

#include "GameLogic/Systems/ShootingSystem.h"

#include "GameData/Components/CharacterStateComponent.generated.h"
#include "GameData/Components/GameplayCommandsComponent.generated.h"
#include "GameData/Components/NetworkEntityIdGeneratorComponent.generated.h"
#include "GameData/Components/TransformComponent.generated.h"
#include "GameData/GameData.h"
#include "GameData/World.h"

#include "Utils/Network/GameplayCommands/CreateProjectileCommand.h"

ShootingSystem::ShootingSystem(WorldHolder& worldHolder, GameStateRewinder& gameStateRewinder) noexcept
	: mWorldHolder(worldHolder)
	, mGameStateRewinder(gameStateRewinder)
{
}

void ShootingSystem::update()
{
	SCOPED_PROFILER("ShootingSystem::update");
	World& world = mWorldHolder.getWorld();

	NetworkEntityIdGeneratorComponent* networkEntityIdGenerator = world.getWorldComponents().getOrAddComponent<NetworkEntityIdGeneratorComponent>();
	GameplayCommandsComponent* gameplayCommands = world.getWorldComponents().getOrAddComponent<GameplayCommandsComponent>();

	world.getEntityManager().forEachComponentSet<const TransformComponent, const CharacterStateComponent>(
		[&gameplayCommands, networkEntityIdGenerator](const TransformComponent* transform, const CharacterStateComponent* characterState)
	{
		if (characterState->getState() == CharacterState::Shoot || characterState->getState() == CharacterState::WalkAndShoot)
		{
			const NetworkEntityId newNetworkId = networkEntityIdGenerator->getGeneratorRef().generateNext();
			gameplayCommands->getDataRef().list.emplace_back(std::make_unique<Network::CreateProjectileCommand>(transform->getLocation(), Vector2D(Rotator((rand() % 360) / 360.0f * (Rotator::MaxValue - Rotator::MinValue) + Rotator::MinValue)), 10.0f, newNetworkId));
		}
	});
}
