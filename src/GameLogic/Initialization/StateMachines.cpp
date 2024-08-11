#include "EngineCommon/precomp.h"

#include "GameLogic/Initialization/StateMachines.h"

namespace StateMachines
{
	static void RegisterCharacterSM(FSM::StateMachine<CharacterState, CharacterStateBlackboardKeys>& sm)
	{
		using FSMType = FSM::StateMachine<CharacterState, CharacterStateBlackboardKeys>;
		{
			FSMType::StateLinkRules rules;
			rules.emplaceLink<FSM::LinkRules::VariableEqualLink, bool>(CharacterState::Walk, CharacterStateBlackboardKeys::TryingToMove, true);
			rules.emplaceLink<FSM::LinkRules::VariableEqualLink, bool>(CharacterState::Shoot, CharacterStateBlackboardKeys::TryingToShoot, true);
			sm.addState(CharacterState::Idle, std::move(rules));
		}
		{
			FSMType::StateLinkRules rules;
			rules.emplaceLink<FSM::LinkRules::VariableEqualLink, bool>(CharacterState::Idle, CharacterStateBlackboardKeys::TryingToMove, false);
			rules.emplaceLink<FSM::LinkRules::VariableEqualLink, bool>(CharacterState::WalkAndShoot, CharacterStateBlackboardKeys::TryingToShoot, true);
			rules.emplaceLink<FSM::LinkRules::VariableEqualLink, bool>(CharacterState::Run, CharacterStateBlackboardKeys::ReadyToRun, true);
			sm.addState(CharacterState::Walk, std::move(rules));
		}
		{
			FSMType::StateLinkRules rules;
			rules.emplaceLink<FSM::LinkRules::VariableEqualLink, bool>(CharacterState::WalkAndShoot, CharacterStateBlackboardKeys::TryingToMove, true);
			rules.emplaceLink<FSM::LinkRules::VariableEqualLink, bool>(CharacterState::Idle, CharacterStateBlackboardKeys::TryingToShoot, false);
			sm.addState(CharacterState::Shoot, std::move(rules));
		}
		{
			FSMType::StateLinkRules rules;
			rules.emplaceLink<FSM::LinkRules::VariableEqualLink, bool>(CharacterState::Shoot, CharacterStateBlackboardKeys::TryingToMove, false);
			rules.emplaceLink<FSM::LinkRules::VariableEqualLink, bool>(CharacterState::Walk, CharacterStateBlackboardKeys::TryingToShoot, false);
			rules.emplaceLink<FSM::LinkRules::VariableEqualLink, bool>(CharacterState::Run, CharacterStateBlackboardKeys::ReadyToRun, true);
			sm.addState(CharacterState::WalkAndShoot, std::move(rules));
		}
		{
			FSMType::StateLinkRules rules;
			rules.emplaceLink<FSM::LinkRules::VariableEqualLink, bool>(CharacterState::Walk, CharacterStateBlackboardKeys::ReadyToRun, false);
			rules.emplaceLink<FSM::LinkRules::VariableEqualLink, bool>(CharacterState::Idle, CharacterStateBlackboardKeys::TryingToMove, false);
			sm.addState(CharacterState::Run, std::move(rules));
		}
	}

	void RegisterStateMachines(StateMachineComponent* stateMachine)
	{
		if (!stateMachine)
		{
			return;
		}

		RegisterCharacterSM(stateMachine->getCharacterSMRef());
	}
} // namespace StateMachines
