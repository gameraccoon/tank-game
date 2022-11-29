#pragma once

#ifndef DEDICATED_SERVER

#include "Utils/Application/ArgumentsParser.h"

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
	GraphicalClient(ApplicationData& applicationData);

	void run(const ArgumentsParser& arguments, const RenderAccessorGameRef& renderAccessorGameRef);

	TankClientGame game;
	HAL::Engine& engine;
};

#endif // !DEDICATED_SERVER
