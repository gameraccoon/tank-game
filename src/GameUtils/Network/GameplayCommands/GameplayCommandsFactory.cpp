#include "EngineCommon/precomp.h"

#include "GameUtils/Network/GameplayCommands/GameplayCommandsFactory.h"

#include <neargye/magic_enum.hpp>

#include "EngineCommon/Types/Serialization.h"

#include "GameUtils/Network/GameplayCommands/GameplayCommandTypes.h"

namespace Network
{
	GameplayCommand::Ptr GameplayCommandFactory::deserialize(const std::vector<std::byte>& stream, size_t& inOutStreamPos) const
	{
		const std::optional<u16> commandTypeIndexOption = Serialization::ReadNumber<u16>(stream, inOutStreamPos);
		if (!commandTypeIndexOption.has_value())
		{
			ReportError("Error while reading command type index");
			return nullptr;
		}

		const u16 commandTypeIndex = *commandTypeIndexOption;

		if (commandTypeIndex >= magic_enum::enum_count<GameplayCommandType>())
		{
			ReportError("Command type out of bounds %u", commandTypeIndex);
			return nullptr;
		}
		const GameplayCommandType type = static_cast<GameplayCommandType>(commandTypeIndex);

		const auto it = mDeserializators.find(type);
		AssertFatal(it != mDeserializators.end(), "Gameplay command wasn't found, id=%u", type);
		return it->second(stream, inOutStreamPos);
	}
} // namespace Network
