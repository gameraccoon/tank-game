#include "EngineCommon/precomp.h"

#include "GameUtils/Network/GameplayCommands/CreatePlayerEntityCommand.h"

#include "EngineCommon/Types/Serialization.h"

#include "GameData/Components/CharacterStateComponent.generated.h"
#include "GameData/Components/ClientGameDataComponent.generated.h"
#include "GameData/Components/CollisionComponent.generated.h"
#include "GameData/Components/MoveInterpolationComponent.generated.h"
#include "GameData/Components/MovementComponent.generated.h"
#include "GameData/Components/NetworkIdComponent.generated.h"
#include "GameData/Components/NetworkIdMappingComponent.generated.h"
#include "GameData/Components/NetworkOwnedEntitiesComponent.generated.h"
#include "GameData/Components/RollbackOnCollisionComponent.generated.h"
#include "GameData/Components/ServerConnectionsComponent.generated.h"
#include "GameData/Components/SpriteCreatorComponent.generated.h"
#include "GameData/Components/TransformComponent.generated.h"
#include "GameData/Components/WeaponComponent.generated.h"
#include "GameData/WorldLayer.h"

#include "GameUtils/Network/GameplayCommands/GameplayCommandTypes.h"
#include "GameUtils/Network/GameStateRewinder.h"

namespace Network
{
	GameplayCommand::Ptr CreatePlayerEntityCommand::createServerSide(const Vector2D pos, const NetworkEntityId networkEntityId, const ConnectionId ownerConnectionId)
	{
		return Ptr(HS_NEW CreatePlayerEntityCommand(pos, networkEntityId, IsOwner::No, ownerConnectionId));
	}

	GameplayCommand::Ptr CreatePlayerEntityCommand::createClientSide(const Vector2D pos, const NetworkEntityId networkEntityId, const IsOwner isOwner)
	{
		return Ptr(HS_NEW CreatePlayerEntityCommand(pos, networkEntityId, isOwner, InvalidConnectionId));
	}

	void CreatePlayerEntityCommand::execute(GameStateRewinder& gameStateRewinder, WorldLayer& world) const
	{
		EntityManager& worldEntityManager = world.getEntityManager();
		Entity controlledEntity = worldEntityManager.addEntity();
		{
			NetworkIdMappingComponent* networkIdMapping = world.getWorldComponents().getOrAddComponent<NetworkIdMappingComponent>();
			networkIdMapping->getNetworkIdToEntityRef().emplace(mNetworkEntityId, controlledEntity);
			networkIdMapping->getEntityToNetworkIdRef().emplace(controlledEntity, mNetworkEntityId);

			TransformComponent* transform = worldEntityManager.addComponent<TransformComponent>(controlledEntity);
			transform->setLocation(mPos);
			transform->setDirection(Vector2D(0.0f, -1.0f));

			MovementComponent* movement = worldEntityManager.addComponent<MovementComponent>(controlledEntity);
			movement->setSpeed(30.0f);

#ifndef DISABLE_SDL
			SpriteCreatorComponent* spriteCreator = worldEntityManager.addComponent<SpriteCreatorComponent>(controlledEntity);
			spriteCreator->getDescriptionsRef().emplace_back(SpriteParams{ Vector2D(16, 16), Vector2D{ 0.5f, 0.5f } }, RelativeResourcePath("resources/textures/tank-enemy-level1-1.png"));
#endif // !DISABLE_SDL

			NetworkIdComponent* networkId = worldEntityManager.addComponent<NetworkIdComponent>(controlledEntity);
			networkId->setId(mNetworkEntityId);

			CollisionComponent* collision = worldEntityManager.addComponent<CollisionComponent>(controlledEntity);
			collision->setBoundingBox(BoundingBox{ Vector2D(-8, -8), Vector2D(8, 8) });

			worldEntityManager.addComponent<CharacterStateComponent>(controlledEntity);
			worldEntityManager.addComponent<WeaponComponent>(controlledEntity);
			worldEntityManager.addComponent<RollbackOnCollisionComponent>(controlledEntity);
		}

		if (gameStateRewinder.isServer())
		{
			ServerConnectionsComponent* serverConnections = gameStateRewinder.getNotRewindableComponents().getOrAddComponent<ServerConnectionsComponent>();
			serverConnections->getClientDataRef()[mOwnerConnectionId].playerEntity = controlledEntity;
		}
		else
		{
			if (mIsOwner == IsOwner::Yes)
			{
				ClientGameDataComponent* clientGameData = world.getWorldComponents().getOrAddComponent<ClientGameDataComponent>();
				clientGameData->setControlledPlayer(controlledEntity);

				NetworkOwnedEntitiesComponent* networkOwnedEntities = world.getWorldComponents().getOrAddComponent<NetworkOwnedEntitiesComponent>();
				networkOwnedEntities->getOwnedEntitiesRef().push_back(mNetworkEntityId);
			}
			worldEntityManager.addComponent<MoveInterpolationComponent>(controlledEntity);
		}

		LogInfo("CreatePlayerEntityCommand executed in update %u for %s", gameStateRewinder.getTimeData().lastFixedUpdateIndex, gameStateRewinder.isServer() ? "server" : "client");
	}

	GameplayCommand::Ptr CreatePlayerEntityCommand::clone() const
	{
		return std::make_unique<CreatePlayerEntityCommand>(*this);
	}

	void CreatePlayerEntityCommand::serverSerialize(WorldLayer& /*world*/, std::vector<std::byte>& inOutStream, ConnectionId receiverConnectionId) const
	{
		inOutStream.reserve(inOutStream.size() + 1 + 8 + 4 * 2);

		Serialization::AppendNumber<u8>(inOutStream, static_cast<u8>(receiverConnectionId == mOwnerConnectionId));
		Serialization::AppendNumber<u64>(inOutStream, mNetworkEntityId);
		Serialization::AppendNumber<f32>(inOutStream, mPos.x);
		Serialization::AppendNumber<f32>(inOutStream, mPos.y);
	}

	GameplayCommand::Ptr CreatePlayerEntityCommand::ClientDeserialize(const std::vector<std::byte>& stream, size_t& inOutCursorPos)
	{
		const IsOwner isOwner = (Serialization::ReadNumber<u8>(stream, inOutCursorPos) != 0) ? IsOwner::Yes : IsOwner::No;
		const NetworkEntityId serverEntityId = Serialization::ReadNumber<u64>(stream, inOutCursorPos).value_or(0.0f);
		const float playerPosX = Serialization::ReadNumber<f32>(stream, inOutCursorPos).value_or(0.0f);
		const float playerPosY = Serialization::ReadNumber<f32>(stream, inOutCursorPos).value_or(0.0f);

		return createClientSide(
			Vector2D(playerPosX, playerPosY),
			serverEntityId,
			isOwner
		);
	}

	GameplayCommandType CreatePlayerEntityCommand::GetType()
	{
		return GameplayCommandType::CreatePlayerEntity;
	}

	CreatePlayerEntityCommand::CreatePlayerEntityCommand(const Vector2D pos, NetworkEntityId networkEntityId, const IsOwner isOwner, const ConnectionId ownerConnectionId)
		: mIsOwner(isOwner)
		, mPos(pos)
		, mNetworkEntityId(networkEntityId)
		, mOwnerConnectionId(ownerConnectionId)
	{
	}
} // namespace Network
