#include "EngineCommon/precomp.h"

#include "GameUtils/Network/GameplayCommands/GameplayCommandFactoryRegistration.h"

#include "GameUtils/Network/GameplayCommands/GameplayCommandsFactory.h"

#include "GameUtils/Network/GameplayCommands/CreatePlayerEntityCommand.h"
#include "GameUtils/Network/GameplayCommands/CreateProjectileCommand.h"

namespace Network
{
	void RegisterGameplayCommands(GameplayCommandFactory& factory)
	{
		factory.RegisterCommand<CreatePlayerEntityCommand>();
		factory.RegisterCommand<CreateProjectileCommand>();
	}
} // namespace Network
