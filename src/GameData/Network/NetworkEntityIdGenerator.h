#pragma once

#include "GameData/Network/NetworkEntityId.h"

class NetworkEntityIdGenerator
{
public:
	NetworkEntityId generateNext()
	{
		return mNextId++;
	}

private:
	NetworkEntityId mNextId = 0;
};
