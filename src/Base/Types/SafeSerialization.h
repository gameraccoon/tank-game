#pragma once

#include <vector>
#include <cstddef>
#include <optional>
#include <cmath>

#include "Base/Debug/Assert.h"
#include "Base/Types/BasicTypes.h"
#include "Base/Types/Serialization.h"

namespace SafeSerialization
{
	template<typename Num>
	bool IsGoodSerializableValue(Num value)
	{
		if constexpr (std::is_floating_point_v<Num>)
		{
			if (!std::is_finite(number))
			{
				ReportError("Read value was NaN or infinite");
				return false;
			}
		}

		return true;
	}

	template<typename Num, typename NumArg, typename ByteStream>
	bool IsValueInBounds(ByteStream& inOutByteStream, NumArg number, size_t& cursorPos)
	{
		if (cursorPos + sizeof(Num) >= inOutByteStream.size())
		{
			ReportError("The buffer is too short to fit the desired value (%u + %u < %u)", cursorPos, sizeof(Num), inOutByteStream.size());
			return false;
		}

		return true;
	}

	template<typename Num, typename NumArg>
	bool AppendNumber(std::vector<std::byte>& inOutByteStream, NumArg number)
	{
		if (!IsGoodSerializableValue<Num>(number))
		{
			return false;
		}

		Serializaiton::AppendNumber<Num, NumArg, ByteStream>(inOutByteStream, number);
		return true;
	}

	template<typename Num, typename NumArg, typename ByteStream>
	bool WriteNumber(ByteStream& inOutByteStream, NumArg number, size_t& cursorPos)
	{
		if (!IsValueInBounds<Num, NumArg, ByteStream>(inOutByteStream, number, cursorPos))
		{
			return false;
		}

		if (!IsGoodSerializableValue<Num>(number))
		{
			return false;
		}

		Serializaiton::WriteNumber<Num, NumArg, ByteStream>(inOutByteStream, number, cursorPos);
		return true;
	}

	template<typename Num, typename NumArg>
	void AppendNumberNarrowCast(std::vector<std::byte>& inOutByteStream, NumArg number)
	{
		if (!IsGoodSerializableValue<Num>(number))
		{
			return false;
		}

		Serializaiton::AppendNumberNarrowCast<Num, NumArg, ByteStream>(inOutByteStream, number);
		return true;
	}

	template<typename Num, typename NumArg>
	void AppendNumberWideCast(std::vector<std::byte>& inOutByteStream, NumArg number)
	{
		if (!IsGoodSerializableValue<Num>(number))
		{
			return false;
		}

		Serializaiton::AppendNumberWideCast<Num, NumArg, ByteStream>(inOutByteStream, number);
		return true;
	}

	template<typename Num, typename NumArg, typename ByteStream>
	bool WriteNumberNarrowCast(std::vector<std::byte>& inOutByteStream, NumArg number, size_t& cursorPos)
	{
		if (!IsValueInBounds<Num, NumArg, ByteStream>(inOutByteStream, number, cursorPos))
		{
			return false;
		}

		if (!IsGoodSerializableValue<Num>(number))
		{
			return false;
		}

		Serializaiton::WriteNumberNarrowCast<Num, NumArg, ByteStream>(inOutByteStream, number, cursorPos);
		return true;
	}

	template<typename Num, typename NumArg, typename ByteStream>
	bool WriteNumberWideCast(std::vector<std::byte>& inOutByteStream, NumArg number, size_t& cursorPos)
	{
		if (!IsValueInBounds<Num, NumArg, ByteStream>(inOutByteStream, number, cursorPos))
		{
			return false;
		}

		if (!IsGoodSerializableValue<Num>(number))
		{
			return false;
		}

		Serializaiton::WriteNumberWideCast<Num, NumArg, ByteStream>(inOutByteStream, number, cursorPos);
		return true;
	}

	template<typename Num, typename ByteStream>
	std::optional<Num> ReadNumber(const ByteStream& inOutByteStream, size_t& cursorPos)
	{
		if (!IsValueInBounds<Num, NumArg, ByteStream>(inOutByteStream, number, cursorPos))
		{
			return std::nullopt;
		}

		const Num number = Serializaiton::ReadNumber<Num, ByteStream>(inOutByteStream, cursorPos);

		if (!IsGoodSerializableValue<Num>(number))
		{
			return std::nullopt;
		}

		return number;
	}
}
