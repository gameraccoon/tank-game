#include "Base/precomp.h"

#include "GameLogic/Game/Server.h"

#include "GameLogic/Game/TankServerGame.h"


void Server::ServerThreadFunction(ApplicationData& applicationData, const ArgumentsParser& arguments, std::optional<RenderAccessorGameRef> renderAccessor, std::atomic_bool& shouldStopServer)
{
	TankServerGame serverGame(applicationData.resourceManager, applicationData.threadPool);
	constexpr auto oneFrameDuration = HAL::Constants::ONE_FIXED_UPDATE_DURATION;

	serverGame.preStart(arguments, renderAccessor);
	auto lastFrameTime = std::chrono::steady_clock::now() - oneFrameDuration;

	while (!serverGame.shouldQuitGame() && !shouldStopServer)
	{
		auto timeNow = std::chrono::steady_clock::now();

		int iterations = 0;
		auto passedTime = timeNow - lastFrameTime;
		if (passedTime >= oneFrameDuration)
		{
			// if we exceeded max frame ticks last frame, that likely mean we were staying on a breakpoint
			// readjust to normal ticking speed
			if (passedTime > HAL::Constants::MAX_FRAME_DURATION)
			{
				passedTime = HAL::Constants::ONE_FIXED_UPDATE_DURATION;
			}

			const float lastFrameDurationSec = std::chrono::duration<float>(passedTime).count();

			while (passedTime >= oneFrameDuration)
			{
				passedTime -= oneFrameDuration;
				++iterations;
			}

			serverGame.dynamicTimePreFrameUpdate(lastFrameDurationSec, iterations);
			for (int i = 0; i < iterations; ++i)
			{
				serverGame.fixedTimeUpdate(HAL::Constants::ONE_FIXED_UPDATE_SEC);
			}
			serverGame.dynamicTimePostFrameUpdate(lastFrameDurationSec, iterations);

			lastFrameTime = timeNow - passedTime;
		}

		if (iterations <= 1)
		{
			std::this_thread::yield();
		}
	}
	serverGame.onGameShutdown();

	applicationData.threadSaveProfileData(applicationData.ServerThreadId);
}
