#include "EngineCommon/precomp.h"

#include "GameLogic/Initialization/StateMachines.h"

#include "GameUtils/Scripting/LuaFunctionCall.h"
#include "GameUtils/Scripting/LuaInstance.h"

namespace StateMachines
{
	static void RegisterCharacterSM(FSM::StateMachine<CharacterState, CharacterStateBlackboardKeys>& sm)
	{
		using FSMType = FSM::StateMachine<CharacterState, CharacterStateBlackboardKeys>;

		LuaInstance luaInstance(LuaInstance::OpenStandardLibs::Yes);

		const AbsoluteResourcePath scriptPath(std::filesystem::current_path(), RelativeResourcePath("resources/scripts/config/state_machines.lua"));
		const LuaExecResult execResult = luaInstance.execScriptFromFile(scriptPath);
		if (execResult.statusCode != 0)
		{
			ReportErrorRelease("Failed to execute state_machines.lua: %s", execResult.errorMessage.c_str());
			return;
		}
		lua_State& luaState = luaInstance.getLuaState();

		LuaInternal::GetGlobal(luaState, "characterFsm");
		if (!LuaInternal::IsTable(luaState, 0))
		{
			ReportErrorRelease("Failed to get characterFsm table from state_machines.lua");
			return;
		}

		// for each state
		// ReSharper disable once CppDeclarationHidesUncapturedLocal
		LuaType::IterateOverTable(luaState, [&sm](lua_State& luaState) {
			CharacterState stateId;
			if (const std::optional<CharacterState> name = LuaType::ReadValue<CharacterState>(luaState, LuaInternal::STACK_TOP - 1))
			{
				stateId = *name;
			}
			else
			{
				ReportErrorRelease("Failed to get state name from a state table in characterFsm table in state_machines.lua");
				return;
			}

			if (!LuaInternal::IsTable(luaState, LuaInternal::STACK_TOP))
			{
				ReportErrorRelease("Failed to get a state table from characterFsm table in state_machines.lua");
				return;
			}

			// for each link
			FSMType::StateLinkRules rules;
			LuaType::IterateOverTable(luaState, [&rules](lua_State& luaState) {
				if (!LuaInternal::IsTable(luaState, LuaInternal::STACK_TOP))
				{
					ReportErrorRelease("Failed to get a link table from a state table in characterFsm table in state_machines.lua");
					return;
				}

				std::string linkType;
				if (std::optional<std::string> type = LuaType::ReadField<std::string>(luaState, "type"))
				{
					linkType = std::move(*type);
				}
				else
				{
					ReportErrorRelease("Failed to get link type from a link table in a state table in characterFsm table in state_machines.lua");
					return;
					;
				}

				if (linkType == "VariableEqualLink")
				{
					const std::optional<CharacterState> targetState = LuaType::ReadField<CharacterState>(luaState, "targetState");
					if (!targetState)
					{
						ReportErrorRelease("Failed to get targetState from a VariableEqualLink table in a state table in characterFsm table in state_machines.lua");
						return;
						;
					}

					const std::optional<CharacterStateBlackboardKeys> variableName = LuaType::ReadField<CharacterStateBlackboardKeys>(luaState, "variableName");
					if (!variableName)
					{
						ReportErrorRelease("Failed to get variableName from a VariableEqualLink table in a state table in characterFsm table in state_machines.lua");
						return;
					}

					const std::optional<bool> value = LuaType::ReadField<bool>(luaState, "value");
					if (!value)
					{
						ReportErrorRelease("Failed to get value from a VariableEqualLink table in a state table in characterFsm table in state_machines.lua");
						return;
					}

					rules.emplaceLink<FSM::LinkRules::VariableEqualLink, bool>(*targetState, *variableName, *value);
				}
				else
				{
					ReportErrorRelease("Unknown link type in a link table in a state table in characterFsm table in state_machines.lua");
				}
			});

			sm.addState(stateId, std::move(rules));
		});
		LuaInternal::Pop(luaState); // pop characterFsm table
	}

	void RegisterStateMachines(StateMachineComponent* stateMachine)
	{
		if (!stateMachine)
		{
			ReportError("StateMachineComponent is nullptr when registering state machines");
			return;
		}

		RegisterCharacterSM(stateMachine->getCharacterSMRef());
	}
} // namespace StateMachines
