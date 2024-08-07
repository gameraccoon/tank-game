#pragma once

#include <optional>

#include "EngineCommon/Types/BasicTypes.h"

#include "GameData/Render/RenderAccessorGameRef.h"

#include "GameUtils/Network/GameStateRewinder.h"

#include "HAL/Network/ConnectionManager.h"

#include "GameLogic/Game/Game.h"

class ArgumentsParser;

class TankServerGame final : public Game
{
public:
	TankServerGame(ResourceManager& resourceManager, ThreadPool& threadPool, int instanceIndex) noexcept;

	void preStart(const ArgumentsParser& arguments, std::optional<RenderAccessorGameRef> renderAccessor);
	void initResources() final;
	void notPausablePreFrameUpdate(float dt) final;
	void dynamicTimePreFrameUpdate(float dt, int plannedFixedTimeUpdates) final;
	void fixedTimeUpdate(float dt) final;
	void notPausablePostFrameUpdate(float dt) final;

	bool shouldPauseGame() const final { return mShouldPauseGame; }
	bool shouldQuitGame() const final { return mShouldQuitGame; }
	void quitGame() final { mShouldQuitGame = true; }
	std::chrono::duration<int64_t, std::micro> getFrameLengthCorrection() const final { return std::chrono::microseconds(0); }

protected:
	TimeData& getTimeData() final;

private:
	void initSystems(bool shouldRender);
	void updateHistory();

private:
	GameStateRewinder mGameStateRewinder{GameStateRewinder::HistoryType::Server, getComponentFactory()};
	HAL::ConnectionManager mConnectionManager;
	bool mShouldPauseGame = false;
	bool mShouldQuitGame = false;
	u16 mServerPort = 14436;
};
