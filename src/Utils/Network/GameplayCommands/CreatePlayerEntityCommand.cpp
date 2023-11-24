#include "Base/precomp.h"

#include "Utils/Network/GameplayCommands/CreatePlayerEntityCommand.h"

#include "Base/Types/Serialization.h"

#include "GameData/Components/CharacterStateComponent.generated.h"
#include "GameData/Components/ClientGameDataComponent.generated.h"
#include "GameData/Components/CollisionComponent.generated.h"
#include "GameData/Components/MovementComponent.generated.h"
#include "GameData/Components/NetworkIdComponent.generated.h"
#include "GameData/Components/NetworkIdMappingComponent.generated.h"
#include "GameData/Components/SpriteCreatorComponent.generated.h"
#include "GameData/Components/RollbackOnCollisionComponent.generated.h"
#include "GameData/Components/TransformComponent.generated.h"
#include "GameData/Components/ServerConnectionsComponent.generated.h"
#include "GameData/Components/WeaponComponent.generated.h"
#include "GameData/World.h"

#include "Utils/Network/GameStateRewinder.h"

#include "Utils/Network/GameplayCommands/GameplayCommandTypes.h"

namespace Network
{
	GameplayCommand::Ptr CreatePlayerEntityCommand::createServerSide(Vector2D pos, NetworkEntityId networkEntityId, ConnectionId ownerConnectionId)
	{
		return GameplayCommand::Ptr(HS_NEW CreatePlayerEntityCommand(pos, networkEntityId, IsOwner::No, ownerConnectionId, NetworkSide::Server));
	}

	GameplayCommand::Ptr CreatePlayerEntityCommand::createClientSide(Vector2D pos, NetworkEntityId networkEntityId, IsOwner isOwner)
	{
		return GameplayCommand::Ptr(HS_NEW CreatePlayerEntityCommand(pos, networkEntityId, isOwner, InvalidConnectionId, NetworkSide::Client));
	}

	void CreatePlayerEntityCommand::execute(GameStateRewinder& gameStateRewinder, World& world) const
	{
		EntityManager& worldEntityManager = world.getEntityManager();
		Entity controlledEntity = worldEntityManager.addEntity();
		{
			NetworkIdMappingComponent* networkIdMapping = world.getWorldComponents().getOrAddComponent<NetworkIdMappingComponent>();
			networkIdMapping->getNetworkIdToEntityRef().emplace(mNetworkEntityId, controlledEntity);
			networkIdMapping->getEntityToNetworkIdRef().emplace(controlledEntity, mNetworkEntityId);

			TransformComponent* transform = worldEntityManager.addComponent<TransformComponent>(controlledEntity);
			transform->setLocation(mPos);
			transform->setDirection(Direction4::Up);

			MovementComponent* movement = worldEntityManager.addComponent<MovementComponent>(controlledEntity);
			movement->setSpeed(0.5f);

#ifndef DEDICATED_SERVER
			SpriteCreatorComponent* spriteCreator = worldEntityManager.addComponent<SpriteCreatorComponent>(controlledEntity);
			spriteCreator->getDescriptionsRef().emplace_back(SpriteParams{Vector2D(16, 16), Vector2D{0.5f, 0.5f}}, RelativeResourcePath("resources/textures/tank-enemy-level1-1.png"));
#endif // !DEDICATED_SERVER

			NetworkIdComponent* networkId = worldEntityManager.addComponent<NetworkIdComponent>(controlledEntity);
			networkId->setId(mNetworkEntityId);

			CollisionComponent* collision = worldEntityManager.addComponent<CollisionComponent>(controlledEntity);
			collision->setBoundingBox(BoundingBox{Vector2D(-8, -8), Vector2D(8, 8)});

			worldEntityManager.addComponent<CharacterStateComponent>(controlledEntity);
			worldEntityManager.addComponent<WeaponComponent>(controlledEntity);
			worldEntityManager.addComponent<RollbackOnCollisionComponent>(controlledEntity);
		}

		if (mNetworkSide == NetworkSide::Server)
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
			}
		}

		LogInfo("CreatePlayerEntityCommand executed in update %u for %s", gameStateRewinder.getTimeData().lastFixedUpdateIndex, mNetworkSide == NetworkSide::Server ? "server" : "client");
	}

	GameplayCommand::Ptr CreatePlayerEntityCommand::clone() const
	{
		return std::make_unique<CreatePlayerEntityCommand>(*this);
	}

	void CreatePlayerEntityCommand::serverSerialize(World& /*world*/, std::vector<std::byte>& inOutStream, ConnectionId receiverConnectionId) const
	{
		inOutStream.reserve(inOutStream.size() + 1 + 8 + 4*2);

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

	CreatePlayerEntityCommand::CreatePlayerEntityCommand(Vector2D pos, NetworkEntityId networkEntityId, IsOwner isOwner, ConnectionId ownerConnectionId, NetworkSide networkSide)
		: mIsOwner(isOwner)
		, mNetworkSide(networkSide)
		, mPos(pos)
		, mNetworkEntityId(networkEntityId)
		, mOwnerConnectionId(ownerConnectionId)
	{
	}
}
