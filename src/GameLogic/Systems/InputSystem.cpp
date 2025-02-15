#include "EngineCommon/precomp.h"

#include "GameLogic/Systems/InputSystem.h"

#ifndef DISABLE_SDL

#include <SDL_scancode.h>

#include "EngineData/Input/InputBindingTypes.h"

#include "GameData/Components/GameplayInputComponent.generated.h"
#include "GameData/Components/ImguiComponent.generated.h"
#include "GameData/Components/TimeComponent.generated.h"
#include "GameData/GameData.h"
#include "GameData/Input/InputBindings.h"
#include "GameData/WorldLayer.h"

#include "HAL/InputControllersData.h"

#include "GameUtils/SharedManagers/WorldHolder.h"

InputSystem::InputSystem(WorldHolder& worldHolder, const HAL::InputControllersData& inputData) noexcept
	: mWorldHolder(worldHolder)
	, mInputData(inputData)
{
}

void InputSystem::update()
{
	SCOPED_PROFILER("InputSystem::update");

#ifdef IMGUI_ENABLED
	GameData& gameData = mWorldHolder.getGameData();
	if (auto [imgui] = gameData.getGameComponents().getComponents<ImguiComponent>(); imgui)
	{
		if (imgui->getIsImguiVisible())
		{
			// stop processing input if imgui is shown
			return;
		}
	}
#endif // IMGUI_ENABLED

	processGameplayInput();
}

static Input::InputBindings GetDebugInputBindings()
{
	using namespace Input;

	InputBindings result;

	// yes, this is not efficient, but this is a temporary solution

	result.keyBindings[GameplayInput::InputKey::MoveUp].push_back(std::make_unique<PressSingleButtonKeyBinding>(ControllerType::Keyboard, SDL_SCANCODE_UP));
	result.keyBindings[GameplayInput::InputKey::MoveUp].push_back(std::make_unique<PressSingleButtonKeyBinding>(ControllerType::Keyboard, SDL_SCANCODE_W));
	result.keyBindings[GameplayInput::InputKey::MoveDown].push_back(std::make_unique<PressSingleButtonKeyBinding>(ControllerType::Keyboard, SDL_SCANCODE_DOWN));
	result.keyBindings[GameplayInput::InputKey::MoveDown].push_back(std::make_unique<PressSingleButtonKeyBinding>(ControllerType::Keyboard, SDL_SCANCODE_S));
	result.keyBindings[GameplayInput::InputKey::MoveLeft].push_back(std::make_unique<PressSingleButtonKeyBinding>(ControllerType::Keyboard, SDL_SCANCODE_LEFT));
	result.keyBindings[GameplayInput::InputKey::MoveLeft].push_back(std::make_unique<PressSingleButtonKeyBinding>(ControllerType::Keyboard, SDL_SCANCODE_A));
	result.keyBindings[GameplayInput::InputKey::MoveRight].push_back(std::make_unique<PressSingleButtonKeyBinding>(ControllerType::Keyboard, SDL_SCANCODE_RIGHT));
	result.keyBindings[GameplayInput::InputKey::MoveRight].push_back(std::make_unique<PressSingleButtonKeyBinding>(ControllerType::Keyboard, SDL_SCANCODE_D));
	result.keyBindings[GameplayInput::InputKey::Shoot].push_back(std::make_unique<PressSingleButtonKeyBinding>(ControllerType::Keyboard, SDL_SCANCODE_SPACE));
	result.keyBindings[GameplayInput::InputKey::Shoot].push_back(std::make_unique<PressSingleButtonKeyBinding>(ControllerType::Keyboard, SDL_SCANCODE_RCTRL));

	return result;
}

void InputSystem::processGameplayInput() const
{
	SCOPED_PROFILER("InputSystem::processGameplayInput");

	using namespace Input;

	WorldLayer& world = mWorldHolder.getDynamicWorldLayer();
	const PlayerControllerStates& controllerStates = mInputData.controllerStates;

	const auto [time] = world.getWorldComponents().getComponents<const TimeComponent>();
	const GameplayTimestamp currentTimestamp = time->getValue()->lastFixedUpdateTimestamp.getIncreasedByUpdateCount(1);

	GameplayInputComponent* gameplayInput = world.getWorldComponents().getOrAddComponent<GameplayInputComponent>();

	GameplayInput::FrameState& gameplayInputState = gameplayInput->getCurrentFrameStateRef();

	// Debug keybindings (static variable just for now, it should be data-driven and stored somewhere)
	static InputBindings gameplayBindings = GetDebugInputBindings();

	for (const auto& pair : gameplayBindings.keyBindings)
	{
		gameplayInputState.updateKey(pair.first, InputBindings::GetKeyState(pair.second, controllerStates), currentTimestamp);
	}

	for (const auto& pair : gameplayBindings.axisBindings)
	{
		gameplayInputState.updateAxis(pair.first, InputBindings::GetBlendedAxisValue(pair.second, controllerStates));
	}
}

#endif // !DISABLE_SDL
