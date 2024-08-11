#include "EngineCommon/precomp.h"

#include "GameUtils/Network/Messages/ServerClient/DisconnectMessage.h"

#include "EngineCommon/Types/Serialization.h"
#include "EngineCommon/Types/String/StringHelpers.h"

#include "GameData/Network/NetworkMessageIds.h"

namespace Network::ServerClient
{
	namespace DisconnectMessageInternal
	{
		template<class... Ts>
		struct Overload : Ts...
		{
			using Ts::operator()...;
		};
		template<class... Ts>
		Overload(Ts...) -> Overload<Ts...>;

		template<typename... Ts>
		void CreateVariant(std::variant<Ts...>& variant, std::size_t i)
		{
			static constexpr std::variant<Ts...> table[] = { Ts{}... };
			variant = table[i];
		}
	}

	std::string ReasonToString(const DisconnectReason::Value& reason)
	{
		using namespace DisconnectMessageInternal;

		return std::visit(Overload{
			[](const DisconnectReason::IncompatibleNetworkProtocolVersion& value)
			{
				return FormatString(
					"Can't connect to the server because of incompatible network protocol version. Client version is %u Server version is %u", value.clientVersion, value.serverVersion
				);
			},
			[](DisconnectReason::ClientShutdown)
			{ return std::string("Client has disconnected, reason: shutdown"); },
			[](DisconnectReason::ServerShutdown)
			{ return std::string("Server has disconnected, reason: shutdown"); },
			[](DisconnectReason::Unknown value)
			{ return FormatString("Disconnected. The disconnect is initiated by the other side. Reason: Unknown(%u)", value); },
		}, reason);
	}

	HAL::Network::Message CreateDisconnectMessage(DisconnectReason::Value reason)
	{
		using namespace DisconnectMessageInternal;

		std::vector<std::byte> connectMessageData;

		Serialization::AppendNumber<u8>(connectMessageData, static_cast<u8>(reason.index()));
		static_assert(std::variant_size_v<DisconnectReason::Value> <= std::numeric_limits<u8>::max(), "u8 is not enough to store the variant index");

		std::visit(Overload{
			[&connectMessageData](const DisconnectReason::IncompatibleNetworkProtocolVersion& value)
			{
				Serialization::AppendNumber<u32>(connectMessageData, value.serverVersion);
				Serialization::AppendNumber<u32>(connectMessageData, value.clientVersion);
			},
			[](auto){},
		}, reason);

		return HAL::Network::Message{
			static_cast<u32>(NetworkMessageId::Disconnect),
			connectMessageData
		};
	}

	DisconnectReason::Value ApplyDisconnectMessage(const HAL::Network::Message& message)
	{
		using namespace DisconnectMessageInternal;

		size_t streamIndex = message.payloadStartPos;
		const u8 reasonIdx = Serialization::ReadNumber<u8>(message.data, streamIndex).value_or(0);

		if (reasonIdx >= std::variant_size_v<DisconnectReason::Value>)
		{
			ReportError("Unknown disconnect reason index %u", reasonIdx);
			return DisconnectReason::Unknown{ reasonIdx };
		}

		DisconnectReason::Value reason;
		CreateVariant(reason, reasonIdx);

		std::visit(Overload{
			[&streamIndex, &message](DisconnectReason::IncompatibleNetworkProtocolVersion& value)
			{
				value.serverVersion = Serialization::ReadNumber<u32>(message.data, streamIndex).value_or(0);
				value.clientVersion = Serialization::ReadNumber<u32>(message.data, streamIndex).value_or(0);
			},
			[reasonIdx](DisconnectReason::Unknown& value)
			{
				value.reasonIdx = reasonIdx;
			},
			[](auto){},
		}, reason);

		return reason;
	}

	static_assert(
		std::is_same_v<std::variant_alternative_t<0, DisconnectReason::Value>, DisconnectReason::IncompatibleNetworkProtocolVersion>,
		"IncompatibleNetworkProtocolVersion should always go first in the DisconnectReason::Value variant"
	);
} // namespace Network::ServerClient
