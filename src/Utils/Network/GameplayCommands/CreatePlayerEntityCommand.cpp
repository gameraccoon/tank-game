#include "Base/precomp.h"

#include "Utils/Network/GameplayCommands/CreatePlayerEntityCommand.h"

#include "GameData/Components/CharacterStateComponent.generated.h"
#include "GameData/Components/ClientGameDataComponent.generated.h"
#include "GameData/Components/InputHistoryComponent.generated.h"
#include "GameData/Components/MovementComponent.generated.h"
#include "GameData/Components/NetworkIdComponent.generated.h"
#include "GameData/Components/NetworkIdMappingComponent.generated.h"
#include "GameData/Components/SpriteCreatorComponent.generated.h"
#include "GameData/Components/TransformComponent.generated.h"
#include "GameData/World.h"

namespace Network
{
	CreatePlayerEntityCommand::CreatePlayerEntityCommand(Vector2D pos, bool isOwner, u64 serverEntityId)
		: mPos(pos)
		, mIsOwner(isOwner)
		, mServerEntityId(serverEntityId)
	{
	}

	void CreatePlayerEntityCommand::execute(World& world) const
	{
		EntityManager& worldEntityManager = world.getEntityManager();
		Entity controlledEntity = worldEntityManager.addEntity();
		{
			TransformComponent* transform = worldEntityManager.addComponent<TransformComponent>(controlledEntity);
			transform->setLocation(mPos);
			MovementComponent* movement = worldEntityManager.addComponent<MovementComponent>(controlledEntity);
			movement->setOriginalSpeed(20.0f);
			SpriteCreatorComponent* spriteCreator = worldEntityManager.addComponent<SpriteCreatorComponent>(controlledEntity);
			spriteCreator->getDescriptionsRef().emplace_back(SpriteParams{Vector2D(16, 16), ZERO_VECTOR}, "resources/textures/tank-enemy-level1-1.png");
			worldEntityManager.addComponent<CharacterStateComponent>(controlledEntity);
			NetworkIdComponent* networkId = worldEntityManager.addComponent<NetworkIdComponent>(controlledEntity);
			networkId->setId(mServerEntityId);
		}
		{
			if (mIsOwner)
			{
				ClientGameDataComponent* clientGameData = world.getWorldComponents().getOrAddComponent<ClientGameDataComponent>();
				clientGameData->setControlledPlayer(controlledEntity);
			}
			NetworkIdMappingComponent* networkIdMapping = world.getWorldComponents().getOrAddComponent<NetworkIdMappingComponent>();
			networkIdMapping->getNetworkIdToEntityRef().emplace(mServerEntityId, controlledEntity);
		}
	}

	GameplayCommand::Ptr CreatePlayerEntityCommand::clone() const
	{
		return std::make_unique<CreatePlayerEntityCommand>(*this);
	}
}
