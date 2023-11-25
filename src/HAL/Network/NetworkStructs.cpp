#include "Base/precomp.h"

#include "HAL/Network/NetworkStructs.h"

#include "Base/Types/Serialization.h"

#include <optional>

namespace HAL
{
	namespace Network
	{
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
