#include "Base/precomp.h"

#include "Base/Types/Serialization.h"

#include "Utils/Network/GameplayCommands/GameplayCommandsFactory.h"

namespace Network
{
	GameplayCommand::Ptr GameplayCommandFactory::deserialize(const std::vector<std::byte>& stream, size_t& inOutStreamPos) const
	{
		GameplayCommandType type = static_cast<GameplayCommandType>(Serialization::ReadNumber<u16>(stream, inOutStreamPos));
		auto it = mDeserializators.find(type);
		AssertFatal(it != mDeserializators.end(), "Gameplay command wasn't found, id=%u", type);
		return it->second(stream, inOutStreamPos);
	}
} // namespace Network
