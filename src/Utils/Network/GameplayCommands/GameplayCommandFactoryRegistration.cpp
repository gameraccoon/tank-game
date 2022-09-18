#include "Base/precomp.h"

#include "Utils/Network/GameplayCommands/GameplayCommandFactoryRegistration.h"

#include "Utils/Network/GameplayCommands/GameplayCommandsFactory.h"

#include "Utils/Network/GameplayCommands/CreatePlayerEntityCommand.h"

namespace Network
{
	void RegisterGameplayCommands(GameplayCommandFactory& factory)
	{
		factory.RegisterCommand<CreatePlayerEntityCommand>();
	}
} // namespace Network
