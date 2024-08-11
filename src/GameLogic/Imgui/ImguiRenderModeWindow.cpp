#include "EngineCommon/precomp.h"

#include "GameLogic/Imgui/ImguiRenderModeWindow.h"

#ifdef IMGUI_ENABLED

#include "imgui/imgui.h"

#include "GameData/Components/RenderModeComponent.generated.h"
#include "GameData/GameData.h"

#include "GameUtils/SharedManagers/WorldHolder.h"

#include "GameLogic/Imgui/ImguiDebugData.h"

void ImguiRenderModeWindow::update(ImguiDebugData& debugData)
{
	GameData& gameData = debugData.worldHolder.getGameData();

	if (isVisible)
	{
		if (auto [renderMode] = gameData.getGameComponents().getComponents<RenderModeComponent>(); renderMode)
		{
			ImGui::Begin("Render modes", &isVisible);

			ImGui::Checkbox("Visible Entities", &renderMode->getIsDrawVisibleEntitiesEnabledRef());
			ImGui::Checkbox("Debug Input", &renderMode->getIsDrawDebugInputEnabledRef());
			ImGui::Checkbox("Debug Collisions", &renderMode->getIsDrawDebugCollisionsEnabledRef());
			ImGui::Checkbox("Debug Primitives", &renderMode->getIsDrawDebugPrimitivesEnabledRef());
			ImGui::Checkbox("Debug Character Info", &renderMode->getIsDrawDebugCharacterInfoEnabledRef());

			ImGui::End();
		}
	}
}

#endif // IMGUI_ENABLED
