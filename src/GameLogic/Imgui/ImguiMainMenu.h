#pragma once

#include "GameLogic/Imgui/ImguiRenderModeWindow.h"
#include "GameLogic/Imgui/ImguiSystemsTimeReportWindow.h"
#include "GameLogic/Imgui/ComponentInspector/ImguiComponentInspectorWindow.h"

class ImguiMainMenu
{
public:
	void update(struct ImguiDebugData& debugData);

private:
	bool mShowImguiDemoWindow = false;
	ImguiRenderModeWindow mRenderModeWindow;
	ImguiSystemsTimeReportWindow mSystemsTimeReportWindow;
	ImguiComponentInspectorWindow mComponentInspectorWindow;
};
