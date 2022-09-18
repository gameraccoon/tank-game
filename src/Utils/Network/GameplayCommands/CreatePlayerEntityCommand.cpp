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
#include "GameData/Components/ServerConnectionsComponent.generated.h"
#include "GameData/World.h"

#include "Utils/Network/GameplayCommands/GameplayCommandTypes.h"

namespace Network
{
	CreatePlayerEntityCommand::CreatePlayerEntityCommand(Vector2D pos, ConnectionId ownerConnectionId, NetworkEntityId networkEntityId)
		: mIsServerSide(true)
		, mPos(pos)
		, mNetworkEntityId(networkEntityId)
		, mOwnerConnectionId(ownerConnectionId)
	{
	}

	CreatePlayerEntityCommand::CreatePlayerEntityCommand(bool isOwner, Vector2D pos, NetworkEntityId networkEntityId)
		: mIsOwner(isOwner)
		, mIsServerSide(false)
		, mPos(pos)
		, mNetworkEntityId(networkEntityId)
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
#ifndef DEDICATED_SERVER
			SpriteCreatorComponent* spriteCreator = worldEntityManager.addComponent<SpriteCreatorComponent>(controlledEntity);
			spriteCreator->getDescriptionsRef().emplace_back(SpriteParams{Vector2D(16, 16), ZERO_VECTOR}, "resources/textures/tank-enemy-level1-1.png");
#endif // !DEDICATED_SERVER
			worldEntityManager.addComponent<CharacterStateComponent>(controlledEntity);
			NetworkIdComponent* networkId = worldEntityManager.addComponent<NetworkIdComponent>(controlledEntity);
			networkId->setId(mNetworkEntityId);
			NetworkIdMappingComponent* networkIdMapping = world.getWorldComponents().getOrAddComponent<NetworkIdMappingComponent>();
			networkIdMapping->getNetworkIdToEntityRef().emplace(mNetworkEntityId, controlledEntity);
			networkIdMapping->getEntityToNetworkIdRef().emplace(controlledEntity, mNetworkEntityId);
		}

		if (mIsServerSide)
		{
			ServerConnectionsComponent* serverConnections = world.getNotRewindableWorldComponents().getOrAddComponent<ServerConnectionsComponent>();
			serverConnections->getControlledPlayersRef()[mOwnerConnectionId] = controlledEntity;
		}
		else
		{
			if (mIsOwner)
			{
				ClientGameDataComponent* clientGameData = world.getWorldComponents().getOrAddComponent<ClientGameDataComponent>();
				clientGameData->setControlledPlayer(controlledEntity);
			}
		}
	}

	GameplayCommand::Ptr CreatePlayerEntityCommand::clone() const
	{
		return std::make_unique<CreatePlayerEntityCommand>(*this);
	}

	void CreatePlayerEntityCommand::serverSerialize(World& /*world*/, std::vector<std::byte>& inOutStream, ConnectionId receiverConnectionId) const
	{
		inOutStream.reserve(inOutStream.size() + 1 + 4 + 8 + 4*2);

		Serialization::AppendNumber<u8>(inOutStream, static_cast<u8>(receiverConnectionId == mOwnerConnectionId));
		Serialization::AppendNumber<u64>(inOutStream, mNetworkEntityId);
		Serialization::AppendNumber<f32>(inOutStream, mPos.x);
		Serialization::AppendNumber<f32>(inOutStream, mPos.y);
	}

	GameplayCommand::Ptr CreatePlayerEntityCommand::ClientDeserialize(const std::vector<std::byte>& stream, size_t& inOutCursorPos)
	{
		const bool isOwner = (Serialization::ReadNumber<u8>(stream, inOutCursorPos) != 0);
		const NetworkEntityId serverEntityId = Serialization::ReadNumber<u64>(stream, inOutCursorPos);
		const float playerPosX = Serialization::ReadNumber<f32>(stream, inOutCursorPos);
		const float playerPosY = Serialization::ReadNumber<f32>(stream, inOutCursorPos);

		return std::make_unique<Network::CreatePlayerEntityCommand>(
			isOwner,
			Vector2D(playerPosX, playerPosY),
			serverEntityId
		);
	}

	GameplayCommandType CreatePlayerEntityCommand::GetType()
	{
		return GameplayCommandType::CreatePlayerEntity;
	}
}
