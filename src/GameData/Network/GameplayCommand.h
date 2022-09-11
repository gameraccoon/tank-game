#pragma once

#include <memory>
#include <vector>

class World;

namespace Network
{
	// a command that is executed from server to client and can
	// be bound to a specific frame in the simulation history
	class GameplayCommand
	{
	public:
		using Ptr = std::unique_ptr<GameplayCommand>;

	public:
		virtual ~GameplayCommand() = default;
		virtual void execute(World& world) const = 0;
		virtual Ptr clone() const = 0;
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
