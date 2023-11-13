#pragma once

#ifndef DEDICATED_SERVER

#include "HAL/Network/ConnectionManager.h"

#include "GameData/Render/RenderAccessorGameRef.h"

#include "Utils/Network/FrameTimeCorrector.h"
#include "Utils/Network/GameStateRewinder.h"

#include "GameLogic/Game/Game.h"
#ifdef IMGUI_ENABLED
#include "GameLogic/Imgui/ImguiDebugData.h"
#endif

class ArgumentsParser;
class GameplayTimestamp;

class TankClientGame final : public Game
{
public:
	using Game::Game;

	void preStart(const ArgumentsParser& arguments, RenderAccessorGameRef renderAccessor);
	void initResources() override;
	void dynamicTimePreFrameUpdate(float dt, int plannedFixedTimeUpdates) final;
	void fixedTimeUpdate(float dt) final;
	void dynamicTimePostFrameUpdate(float dt, int processedFixedTimeUpdates) final;

	bool shouldQuitGame() const final { return mShouldQuitGame; }
	void quitGame() final { mShouldQuitGameNextTick = true; }
	std::chrono::duration<int64_t, std::micro> getFrameLengthCorrection() const final;

protected:
	TimeData& getTimeData() final;

private:
	void initSystems();
	void correctUpdates(u32 firstUpdateToResimulateIdx);
	void removeOldUpdates();
	void processCorrections();

private:
	GameStateRewinder mGameStateRewinder{GameStateRewinder::HistoryType::Client , getComponentFactory(), getEntityGenerator()};
	HAL::ConnectionManager mConnectionManager;
	bool mShouldQuitGameNextTick = false;
	bool mShouldQuitGame = false;
	FrameTimeCorrector mFrameTimeCorrector;

	HAL::ConnectionManager::NetworkAddress mServerAddress{HAL::ConnectionManager::NetworkAddress::Ipv4({127, 0, 0, 1}, 14436)};

#ifdef IMGUI_ENABLED
	ImguiDebugData mImguiDebugData{getWorldHolder(), getComponentFactory(), {}, {}};
#endif // IMGUI_ENABLED
};

#endif // !DEDICATED_SERVER
