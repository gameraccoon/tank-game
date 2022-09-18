#pragma once

#include <memory>
#include <vector>

#include "GameData/Network/ConnectionId.h"

class World;

namespace Network
{
	enum class GameplayCommandType : u16;

	// a command that is executed from server to client and can
	// be bound to a specific frame in the simulation history
	class GameplayCommand
	{
	public:
		using Ptr = std::unique_ptr<GameplayCommand>;

	public:
		virtual ~GameplayCommand() = default;
		virtual GameplayCommandType getType() const = 0;
		virtual void execute(World& world) const = 0;
		virtual Ptr clone() const = 0;
		virtual void serverSerialize(World& world, std::vector<std::byte>& inOutStream, ConnectionId receiverConnectionId) const = 0;
	};

	struct GameplayCommandList
	{
		std::vector<GameplayCommand::Ptr> list;

		GameplayCommandList() = default;
		GameplayCommandList(const GameplayCommandList&);
		GameplayCommandList& operator=(const GameplayCommandList&) ;
		GameplayCommandList(GameplayCommandList&&) = default;
		GameplayCommandList& operator=(GameplayCommandList&&) = default;
		~GameplayCommandList() = default;
	};
}
