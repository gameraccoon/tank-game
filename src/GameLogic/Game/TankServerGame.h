#pragma once

#include <optional>

#include "Base/Types/BasicTypes.h"

#include "GameData/Render/RenderAccessorGameRef.h"

#include "HAL/Network/ConnectionManager.h"

#include "GameLogic/Game/Game.h"
#ifdef IMGUI_ENABLED
#include "GameLogic/Imgui/ImguiDebugData.h"
#endif

class ArgumentsParser;

class TankServerGame : public Game
{
public:
	TankServerGame(ResourceManager& resourceManager, ThreadPool& threadPool);

	void preStart(const ArgumentsParser& arguments, std::optional<RenderAccessorGameRef> renderAccessor);
	void initResources() final;
	void dynamicTimePreFrameUpdate(float dt, int plannedFixedTimeUpdates) final;
	void fixedTimeUpdate(float dt) final;
	void dynamicTimePostFrameUpdate(float dt, int processedFixedTimeUpdates) final;

	bool shouldQuitGame() const override { return mShouldQuitGame; }
	void quitGame() override { mShouldQuitGame = true; }

private:
	void initSystems(bool shouldRender);
	void correctUpdates(u32 firstIncorrectUpdateIdx);
	void processInputCorrections();
	void saveInputForLastFrame(u32 inputUpdateIndex);

private:
	HAL::ConnectionManager mConnectionManager;
	bool mShouldQuitGame = false;
};
