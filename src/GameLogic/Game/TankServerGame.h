#pragma once

#include <optional>

#include "EngineCommon/Types/BasicTypes.h"

#include "GameData/Render/RenderAccessorGameRef.h"

#include "GameUtils/Network/GameStateRewinder.h"

#include "GameLogic/Game/Game.h"

class ArgumentsParser;

class TankServerGame final : public Game
{
public:
	TankServerGame(ResourceManager& resourceManager, ThreadPool& threadPool, int instanceIndex) noexcept;

	void preStart(const ArgumentsParser& arguments, std::optional<RenderAccessorGameRef> renderAccessor);
	void initResources() override;
	void notPausablePreFrameUpdate(float dt) override;
	void dynamicTimePreFrameUpdate(float dt, int plannedFixedTimeUpdates) override;
	void fixedTimeUpdate(float dt) override;
	void notPausableRenderUpdate(float frameAlpha) override;

	bool shouldPauseGame() const override { return mShouldPauseGame; }
	bool shouldQuitGame() const override { return mShouldQuitGame; }
	void quitGame() override { mShouldQuitGame = true; }
	std::chrono::duration<int64_t, std::micro> getFrameLengthCorrection() const override { return std::chrono::microseconds(0); }

protected:
	TimeData& getTimeData() override;

private:
	void initSystems(bool shouldRender);
	void consumeNetworkMessages();
	void updateHistory();

private:
	GameStateRewinder mGameStateRewinder{ GameStateRewinder::HistoryType::Server, getComponentFactory() };
	bool mShouldPauseGame = false;
	bool mShouldQuitGame = false;
	u16 mServerPort = 14436;
};
