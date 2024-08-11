#pragma once

#include "GameData/Components/StateMachineComponent.generated.h"
#include "GameData/FSM/StateMachine.h"

namespace StateMachines
{
	// ToDo: need an editor not to hardcode SM data
	void RegisterStateMachines(StateMachineComponent* stateMachine);
}
