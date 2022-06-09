#pragma once

#include <array>
#include <bit>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace Serialization
{
	using ByteStream = std::vector<std::byte>;

	template<typename Int, typename IntArg>
	void WriteNumber(std::vector<std::byte>& inOutByteStream, IntArg integer)
	{
		static_assert(std::is_convertible_v<IntArg, Int>, "Argument type should be convertible to the data type");
		static_assert(std::is_arithmetic_v<Int>, "Type should be ariphmetic to be serialized with WriteNumber");
		static_assert(sizeof(std::array<std::byte, sizeof(Int)>) == sizeof(Int), "Unexpected std::array layout");
		static_assert(std::is_standard_layout_v<std::array<std::byte, sizeof(Int)>>, "Unexpected std::array layout");
		static_assert(std::is_trivially_constructible_v<std::array<std::byte, sizeof(Int)>>, "Unexpected std::array implementation");
		static_assert(std::is_trivially_copyable_v<std::array<std::byte, sizeof(Int)>>, "Unexpected std::array implementation");

		// to allow type conversion without doing error-prone static_cast with duplicating the type
		const Int value = static_cast<Int>(integer);

		const auto* byteRepresentation = std::bit_cast<std::array<std::byte, sizeof(Int)>*>(&value);

		if constexpr (std::endian::native == std::endian::little)
		{
			inOutByteStream.insert(
				inOutByteStream.end(),
				std::begin(*byteRepresentation),
				std::end(*byteRepresentation)
			);
		}
		else if constexpr (std::endian::native == std::endian::big)
		{
			inOutByteStream.insert(
				inOutByteStream.end(),
				std::rbegin(*byteRepresentation),
				std::rend(*byteRepresentation)
			);
		}
		else
		{
			throw std::logic_error("Mixed entian is not supported");
		}
	}

	template<typename Int>
	Int ReadNumber(const ByteStream& inOutByteStream, size_t& cursorPos)
	{
		static_assert(std::is_arithmetic_v<Int>, "Type should be ariphmetic to be deserialized with ReadNumber");
		static_assert(sizeof(std::array<std::byte, sizeof(Int)>) == sizeof(Int), "Unexpected std::array layout");
		static_assert(std::is_standard_layout_v<std::array<std::byte, sizeof(Int)>>, "Unexpected std::array layout");
		static_assert(std::is_trivially_constructible_v<std::array<std::byte, sizeof(Int)>>, "Unexpected std::array implementation");
		static_assert(std::is_trivially_copyable_v<std::array<std::byte, sizeof(Int)>>, "Unexpected std::array implementation");

		Int integer;
		auto* byteRepresentation = std::bit_cast<std::array<std::byte, sizeof(Int)>*>(&integer);

		const size_t lastCursorPos = cursorPos;

		if constexpr (std::endian::native == std::endian::little)
		{
			std::copy(
				inOutByteStream.begin() + lastCursorPos,
				inOutByteStream.begin() + (lastCursorPos + sizeof(Int)),
				byteRepresentation->begin()
			);
		}
		else if constexpr (std::endian::native == std::endian::big)
		{
			std::copy(
				inOutByteStream.rbegin() + (inOutByteStream.size() - lastCursorPos - sizeof(Int)),
				inOutByteStream.rbegin() + (inOutByteStream.size() - lastCursorPos),
				byteRepresentation->begin()
			);
		}
		else
		{
			throw std::logic_error("Mixed entian is not supported");
		}

		cursorPos += sizeof(Int);
		return integer;
	}
}
