#pragma once

#ifndef DEDICATED_SERVER

#include "GameUtils/Application/ArgumentsParser.h"

#include "GameData/Render/RenderAccessorGameRef.h"

#include "GameLogic/Game/TankClientGame.h"

class ApplicationData;
class ArgumentsParser;
class RenderAccessor;

namespace HAL
{
	class Engine;
}

struct GraphicalClient
{
	explicit GraphicalClient(ApplicationData& applicationData, int instanceIndex);

	void run(const ArgumentsParser& arguments, const RenderAccessorGameRef& renderAccessorGameRef);

	TankClientGame game;
	HAL::Engine& engine;
};

#endif // !DEDICATED_SERVER
