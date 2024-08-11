#pragma once

#include <map>

#include <raccoon-ecs/entity.h>

#include "GameLogic/Imgui/ComponentInspector/AbstractComponentImguiWidget.h"
#include "GameLogic/Imgui/ComponentInspector/PropertyFilters/ImguiPropertyFiltersWidget.h"

class WorldHolder;
class WorldCell;
struct ImguiDebugData;

class ImguiComponentInspectorWindow
{
public:
	ImguiComponentInspectorWindow();

	void update(ImguiDebugData& debugData);

	bool isVisible = false;

private:
	void applyFilters(ImguiDebugData& debugData);

	void showEntityId();
	void showFilteredEntities();
	void showComponentsInspector(ImguiDebugData& debugData);

private:
	char mEntityFilterBuffer[128] = "";
	OptionalEntity mSelectedEntity;
	std::vector<Entity> mFilteredEntities;
	std::map<StringId, std::unique_ptr<AbstractComponentImguiWidget>> mComponentInspectWidgets;
	ImguiPropertyFiltration::ImguiPropertyFiltersWidget mPropertyFiltersWidget;
};
