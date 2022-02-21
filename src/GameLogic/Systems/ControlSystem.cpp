#include "Base/precomp.h"

#include "GameLogic/Systems/ControlSystem.h"

#include <sdl/SDL_keycode.h>
#include <sdl/SDL_mouse.h>

#include "GameData/World.h"
#include "GameData/GameData.h"
#include "GameData/Components/TransformComponent.generated.h"
#include "GameData/Components/MovementComponent.generated.h"
#include "GameData/Components/RenderModeComponent.generated.h"
#include "GameData/Components/ImguiComponent.generated.h"
#include "GameData/Components/CharacterStateComponent.generated.h"


ControlSystem::ControlSystem(WorldHolder& worldHolder, const InputData& inputData) noexcept
	: mWorldHolder(worldHolder)
	, mInputData(inputData)
{
}

void UpdateRenderStateOnPressed(const HAL::KeyStatesMap& keys, int key, bool& value)
{
	if (keys.isJustPressed(key))
	{
		value = !value;
	}
}

void ControlSystem::update()
{
	SCOPED_PROFILER("ControlSystem::update");
	GameData& gameData = mWorldHolder.getGameData();

	const HAL::KeyStatesMap& keyStates = mInputData.keyboardKeyStates;

#ifdef IMGUI_ENABLED
	if (auto [imgui] = gameData.getGameComponents().getComponents<ImguiComponent>(); imgui)
	{
		UpdateRenderStateOnPressed(keyStates, SDLK_F1, imgui->getIsImguiVisibleRef());
		if (imgui->getIsImguiVisible())
		{
			// stop processing input if imgui is shown
			return;
		}
	}
#endif // IMGUI_ENABLED

	processPlayerInput();

	if (auto [renderMode] = gameData.getGameComponents().getComponents<RenderModeComponent>(); renderMode)
	{
		UpdateRenderStateOnPressed(keyStates, SDLK_F2, renderMode->getIsDrawDebugCollisionsEnabledRef());
		UpdateRenderStateOnPressed(keyStates, SDLK_F3, renderMode->getIsDrawBackgroundEnabledRef());
		UpdateRenderStateOnPressed(keyStates, SDLK_F4, renderMode->getIsDrawLightsEnabledRef());
		UpdateRenderStateOnPressed(keyStates, SDLK_F5, renderMode->getIsDrawVisibleEntitiesEnabledRef());
		UpdateRenderStateOnPressed(keyStates, SDLK_F7, renderMode->getIsDrawDebugCharacterInfoEnabledRef());
		UpdateRenderStateOnPressed(keyStates, SDLK_F8, renderMode->getIsDrawDebugPrimitivesEnabledRef());
		UpdateRenderStateOnPressed(keyStates, SDLK_F9, renderMode->getIsDrawDebugCellInfoEnabledRef());
	}
}

void ControlSystem::processPlayerInput()
{
	const HAL::KeyStatesMap& keyStates = mInputData.keyboardKeyStates;
	//const HAL::KeyStatesMap& mouseKeyStates = mInputData.mouseKeyStates;
	//World& world = mWorldHolder.getWorld();

	//bool isRunPressed = keyStates.isPressed(SDLK_LSHIFT) || keyStates.isPressed(SDLK_RSHIFT);

	//bool isShootPressed = mouseKeyStates.isPressed(SDL_BUTTON_LEFT) || keyStates.isPressed(SDLK_RCTRL);

	Vector2D movementDirection(0.0f, 0.0f);
	if (keyStates.isPressed(SDLK_LEFT) || keyStates.isPressed(SDLK_a))
	{
		movementDirection += LEFT_DIRECTION;
	}

	if (keyStates.isPressed(SDLK_RIGHT) || keyStates.isPressed(SDLK_d))
	{
		movementDirection += RIGHT_DIRECTION;
	}

	if (keyStates.isPressed(SDLK_UP) || keyStates.isPressed(SDLK_w))
	{
		movementDirection += UP_DIRECTION;
	}

	if (keyStates.isPressed(SDLK_DOWN) || keyStates.isPressed(SDLK_s))
	{
		movementDirection += DOWN_DIRECTION;
	}
}
