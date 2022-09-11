#include "Base/precomp.h"

#include "GameData/Network/GameplayCommand.h"

namespace Network
{
	GameplayCommandList::GameplayCommandList(const GameplayCommandList& other)
	{
		list.reserve(other.list.size());
		for (const GameplayCommand::Ptr& command : other.list)
		{
			list.push_back(command->clone());
		}
	}

	GameplayCommandList& GameplayCommandList::operator=(const GameplayCommandList& other)
	{
		list.clear();
		list.reserve(other.list.size());
		for (const GameplayCommand::Ptr& command : other.list)
		{
			list.push_back(command->clone());
		}
		return *this;
	}
} // namespace Network
