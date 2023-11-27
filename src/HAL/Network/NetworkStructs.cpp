#include "Base/precomp.h"

#include "HAL/Network/NetworkStructs.h"

#include "Base/Types/Serialization.h"

#include <optional>

namespace HAL
{
	namespace Network
	{
#ifndef FAKE_NETWORK
		struct NetworkAddress::Impl
		{
			SteamNetworkingIPAddr addr;
		};

		NetworkAddress::NetworkAddress(std::unique_ptr<Impl>&& pimpl)
			: mPimpl(std::move(pimpl))
		{
		}

		NetworkAddress::NetworkAddress(const NetworkAddress& other)
			: mPimpl(std::make_unique<NetworkAddress::Impl>(other.mPimpl->addr))
		{
		}

		NetworkAddress& NetworkAddress::operator=(const NetworkAddress& other)
		{
			mPimpl->addr = other.mPimpl->addr;
			return *this;
		}

		NetworkAddress::~NetworkAddress() = default;

		std::optional<NetworkAddress> NetworkAddress::FromString(const std::string& str)
		{
			SteamNetworkingIPAddr addr;
			if (SteamNetworkingIPAddr_ParseString(&addr, str.c_str()))
			{
				return NetworkAddress(std::make_unique<NetworkAddress::Impl>(addr));
			}

			return std::nullopt;
		}

		NetworkAddress NetworkAddress::Ipv4(std::array<u8, 4> address, u16 port)
		{
			SteamNetworkingIPAddr addr;
			const uint32 ipAddressNumber = (address[0] << (3 * 8)) + (address[1] << (2 * 8)) + (address[2] << 8) + (address[3]);
			addr.SetIPv4(ipAddressNumber, port);
			return NetworkAddress{ std::make_unique<NetworkAddress::Impl>(addr) };
		}

		NetworkAddress NetworkAddress::Ipv6(std::array<u8, 16> address, u16 port)
		{
			SteamNetworkingIPAddr addr;
			addr.SetIPv6(address.data(), port);
			return NetworkAddress{ std::make_unique<NetworkAddress::Impl>(addr) };
		}

		const SteamNetworkingIPAddr& NetworkAddress::getInternalAddress() const
		{
			return mPimpl->addr;
		}
#else
		struct NetworkAddress::Impl
		{
			std::vector<std::byte> addr;
		};

		NetworkAddress::NetworkAddress(const NetworkAddress& other)
			: mPimpl(std::make_unique<NetworkAddress::Impl>(*other.mPimpl))
		{
		}

		NetworkAddress& NetworkAddress::operator=(const NetworkAddress& other)
		{
			mPimpl->addr = other.mPimpl->addr;
			return *this;
		}

		NetworkAddress::~NetworkAddress() = default;

		std::optional<NetworkAddress> NetworkAddress::FromString(const std::string& str)
		{
			NetworkAddress result{std::make_unique<NetworkAddress::Impl>()};
			// split string by ':'
			auto colonPos = str.find(':');
			std::string ipStr = str.substr(0, colonPos);
			std::string portStr = str.substr(colonPos + 1);
			std::string::size_type pos = 0;
			std::string::size_type prev = 0;
			while ((pos = ipStr.find('.', prev)) != std::string::npos)
			{
				result.mPimpl->addr.push_back(static_cast<std::byte>(atoi(ipStr.substr(prev, pos - prev).c_str())));
				prev = pos + 1;
			}
			result.mPimpl->addr.push_back(static_cast<std::byte>(atoi(ipStr.substr(prev, pos - prev).c_str())));

			result.mPimpl->addr.push_back(static_cast<std::byte>(atoi(portStr.c_str()) & 0xFF));
			result.mPimpl->addr.push_back(static_cast<std::byte>((atoi(portStr.c_str()) >> 8) & 0xFF));

			return result;
		}

		NetworkAddress NetworkAddress::Ipv4(std::array<u8, 4> address, u16 port)
		{
			std::vector<std::byte> data;
			data.reserve(4 + 2);
			std::transform(address.begin(), address.end(), std::back_inserter(data), [](u8 byte) { return static_cast<std::byte>(byte); });
			data.push_back(static_cast<std::byte>(port & 0xFF));
			data.push_back(static_cast<std::byte>((port >> 8) & 0xFF));
			return NetworkAddress{ std::make_unique<NetworkAddress::Impl>(std::move(data)) };
		}

		NetworkAddress NetworkAddress::Ipv6(std::array<u8, 16> /*address*/, u16 /*port*/)
		{
			ReportFatalError("Not implemented");
			return NetworkAddress({});
		}

		NetworkAddress::NetworkAddress(std::unique_ptr<Impl>&& pimpl)
			: mPimpl(std::move(pimpl))
		{
		}

		bool NetworkAddress::isLocalhost() const
		{
			if (mPimpl->addr.size() == 6)
			{
				return mPimpl->addr[0] == std::byte{ 127 } && mPimpl->addr[1] == std::byte{ 0 } && mPimpl->addr[2] == std::byte{ 0 } && mPimpl->addr[3] == std::byte{ 1 };
			}
			// ToDo: IPv6
			return false;
		}

		u16 NetworkAddress::getPort() const
		{
			if (mPimpl->addr.size() < 6)
			{
				return 0;
			}
			return static_cast<u16>(mPimpl->addr[4]) + (static_cast<u16>(mPimpl->addr[5]) << 8);
		}

#endif // FAKE_NETWORK

		Message::Message(u32 type)
		{
			setMessageType(type);
		}

		Message::Message(std::byte* rawData, size_t rawDataSize)
			: data(rawData, rawData + rawDataSize)
			, cursorPos(rawDataSize)
		{
		}

		Message::Message(u32 type, const std::vector<std::byte>& payload)
		{
			resize(payload.size());
			setMessageType(type);
			std::copy(payload.begin(), payload.end(), data.begin() + payloadStartPos);
		}

		void Message::resize(size_t payloadSize)
		{
			data.resize(headerSize + payloadSize);
		}

		void Message::reserve(size_t payloadSize)
		{
			data.reserve(headerSize + payloadSize);
		}

		void Message::setMessageType(u32 type)
		{
			if (data.size() < headerSize)
			{
				resize(0);
			}

			size_t headerCursorPos = 0;
			Serialization::WriteNumber<u32>(data, type, headerCursorPos);
		}

		u32 Message::readMessageType() const
		{
			size_t headerCursorPos = 0;
			return Serialization::ReadNumber<u32>(data, headerCursorPos).value_or(0);
		}
	} // namespace Network
} // namespace HAL
