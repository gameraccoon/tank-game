#pragma once

#include <optional>

#include "GameData/Render/RenderAccessorGameRef.h"

#include "GameLogic/Game/ApplicationData.h"

class ArgumentsParser;

namespace Server
{
	void ServerThreadFunction(ApplicationData& applicationData, const ArgumentsParser& arguments, std::optional<RenderAccessorGameRef> renderAccessor, std::atomic_bool& shouldStopServer);
};
