#pragma once

#include "GameData/Network/GameplayCommand.h"

class World;
class GameplayCommandHistoryComponent;

namespace GameplayCommandUtils
{
	void AppendFrameToHistory(GameplayCommandHistoryComponent* commandHistory, u32 frameIndex);
	void AddCommandToHistory(World& world, u32 creationFrameIndex, Network::GameplayCommand::Ptr&& newCommand);
	void AddConfirmedSnapshotToHistory(World& world, u32 creationFrameIndex, std::vector<Network::GameplayCommand::Ptr>&& newCommands);
} // namespace GameplayCommandUtils
