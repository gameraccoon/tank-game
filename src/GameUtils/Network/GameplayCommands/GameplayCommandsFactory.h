#pragma once

#include <unordered_map>
#include <functional>
#include <vector>

#include "EngineCommon/Types/BasicTypes.h"

#include "GameData/Network/GameplayCommand.h"

enum class GameplayCommandType : u16;

namespace Network
{
	class GameplayCommandFactory
	{
	public:
		template<typename Command>
		void RegisterCommand()
		{
			GameplayCommandType type = Command::GetType();
			mDeserializators.emplace(type, [](const std::vector<std::byte>& stream, size_t& inOutStreamPos) -> GameplayCommand::Ptr {
				return Command::ClientDeserialize(stream, inOutStreamPos);
			});
		}

		GameplayCommand::Ptr deserialize(const std::vector<std::byte>& stream, size_t& inOutStreamPos) const;

	private:
		std::unordered_map<GameplayCommandType, std::function<GameplayCommand::Ptr(const std::vector<std::byte>&, size_t&)>> mDeserializators;
	};
} // namespace Network
