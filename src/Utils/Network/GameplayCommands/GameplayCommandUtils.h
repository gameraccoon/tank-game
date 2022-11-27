#pragma once

#include "GameData/Network/GameplayCommand.h"

class World;
class GameplayCommandHistoryComponent;
class GameStateRewinder;

namespace GameplayCommandUtils
{
	void AppendFrameToHistory(GameplayCommandHistoryComponent* commandHistory, u32 frameIndex);
	void AddCommandToHistory(GameStateRewinder& stateRewinder, u32 creationFrameIndex, Network::GameplayCommand::Ptr&& newCommand);
	void AddConfirmedSnapshotToHistory(GameStateRewinder& stateRewinder, u32 creationFrameIndex, std::vector<Network::GameplayCommand::Ptr>&& newCommands);
	void AddOverwritingSnapshotToHistory(GameStateRewinder& stateRewinder, u32 creationFrameIndex, std::vector<Network::GameplayCommand::Ptr>&& newCommands);
} // namespace GameplayCommandUtils
