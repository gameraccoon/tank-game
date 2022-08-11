#pragma once

#ifndef DEDICATED_SERVER

#include "Utils/Application/ArgumentsParser.h"

#include "GameData/Render/RenderAccessorGameRef.h"

#include "GameLogic/Game/TankClientGame.h"

class ApplicationData;
class ArgumentsParser;
class RenderAccessor;

struct GraphicalClient
{
	GraphicalClient(ApplicationData& applicationData);

	void run(ArgumentsParser& arguments, const RenderAccessorGameRef& renderAccessorGameRef);

	TankClientGame game;
};

#endif // !DEDICATED_SERVER
