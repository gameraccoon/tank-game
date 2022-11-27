#pragma once

#include "GameData/EcsDefinitions.h"
#include "GameData/Input/InputHistory.h"
#include "GameData/Network/GameplayCommand.h"
#include "GameData/Network/MovementHistory.h"
#include "GameData/World.h"

class WorldHolder;

namespace RaccoonEcs
{
	class EntityGenerator;
}

class GameStateRewinder
{
public:
	using FixedTimeUpdateFn = std::function<void(float)>;

public:
	GameStateRewinder(ComponentFactory& componentFactory, RaccoonEcs::EntityGenerator& entityGenerator, WorldHolder& worldHolderRef);

	World& getWorld() { return *mFrameHistory[mCurrentFrameIdx]; }
	const World& getWorld() const { return *mFrameHistory[mCurrentFrameIdx]; }

	ComponentSetHolder& getNotRewindableComponents() { return mNotRewindableComponents; }
	const ComponentSetHolder& getNotRewindableComponents() const { return mNotRewindableComponents; }

	void addNewFrameToTheHistory();
	void trimOldFrames(size_t oldFramesLeft);
	size_t getStoredFramesCount() const;

	void unwindBackInHistory(size_t framesBackCount);

private:
	ComponentSetHolder mNotRewindableComponents;
	size_t mCurrentFrameIdx = 0;
//	u32 mLastStoredUpdateIdx = 0;
//	u32 mLastDesynchronizedUpdateIdx = std::numeric_limits<u32>::max();

	std::vector<std::unique_ptr<World>> mFrameHistory;
//	Input::InputHistory mInputHistory;
//	MovementHistory mMovesHistory;
//	std::vector<Network::GameplayCommandList> mCommandsHistory;

//	FixedTimeUpdateFn mFixedTimeUpdateFn;
	WorldHolder& mWorldHolderRef;
};
