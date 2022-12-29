#include "Base/precomp.h"

#include "Utils/Network/Messages/ConnectMessage.h"

#include "Base/Types/BasicTypes.h"
#include "Base/Types/Serialization.h"

#include "GameData/Components/CharacterStateComponent.generated.h"
#include "GameData/Components/MovementComponent.generated.h"
#include "GameData/Components/ServerConnectionsComponent.generated.h"
#include "GameData/Components/SpriteCreatorComponent.generated.h"
#include "GameData/Components/TimeComponent.generated.h"
#include "GameData/Components/TransformComponent.generated.h"
#include "GameData/EcsDefinitions.h"
#include "GameData/Network/NetworkMessageIds.h"
#include "GameData/Network/NetworkProtocolVersion.h"
#include "GameData/World.h"

#include "Utils/Network/GameStateRewinder.h"

namespace Network
{
	HAL::ConnectionManager::Message CreateConnectMessage(World& world)
	{
		std::vector<std::byte> connectMessageData;

		Serialization::AppendNumber<u32>(connectMessageData, Network::NetworkProtocolVersion);

		const auto [time] = world.getWorldComponents().getComponents<const TimeComponent>();
		AssertFatal(time, "TimeComponent should be created before the game run");
		const TimeData& timeValue = *time->getValue();

		Serialization::AppendNumber<u32>(connectMessageData, timeValue.lastFixedUpdateIndex);

		return HAL::ConnectionManager::Message{
			static_cast<u32>(NetworkMessageId::Connect),
			std::move(connectMessageData)
		};
	}

	u32 ApplyConnectMessage(World& world, GameStateRewinder& gameStateRewinder, const HAL::ConnectionManager::Message& message, ConnectionId connectionId)
	{
		size_t streamIndex = HAL::ConnectionManager::Message::payloadStartPos;
		const u32 clientNetworkProtocolVersion = Serialization::ReadNumber<u32>(message.data, streamIndex);

		if (clientNetworkProtocolVersion != Network::NetworkProtocolVersion)
		{
			return clientNetworkProtocolVersion;
		}

		const u32 clientFrameIndex = Serialization::ReadNumber<u32>(message.data, streamIndex);

		ServerConnectionsComponent* serverConnections = gameStateRewinder.getNotRewindableComponents().getOrAddComponent<ServerConnectionsComponent>();
		serverConnections->getControlledPlayersRef().emplace(connectionId, OptionalEntity{});

		const auto [time] = world.getWorldComponents().getComponents<const TimeComponent>();
		AssertFatal(time, "TimeComponent should be created before the game run");
		Input::InputHistory& inputHistory = serverConnections->getInputsRef()[connectionId];
		inputHistory.indexShift = static_cast<s32>(time->getValue()->lastFixedUpdateIndex) - clientFrameIndex + 1;

		return clientNetworkProtocolVersion;
	}
}
