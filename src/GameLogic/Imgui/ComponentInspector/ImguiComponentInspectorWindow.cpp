#include "EngineCommon/precomp.h"

#include "GameLogic/Imgui/ComponentInspector/ImguiComponentInspectorWindow.h"

#include <algorithm>
#include <cstring>
#include <ranges>
#include <sstream>
#include <string_view>

#include "imgui/imgui.h"

#include "GameData/GameData.h"
#include "GameData/WorldLayer.h"

#include "GameUtils/SharedManagers/WorldHolder.h"

#include "GameLogic/Imgui/ComponentInspector/ComponentWidgetsRegistration.h"
#include "GameLogic/Imgui/ImguiDebugData.h"

ImguiComponentInspectorWindow::ImguiComponentInspectorWindow()
{
	ComponentWidgetsRegistration::RegisterInspectWidgets(mComponentInspectWidgets);
}

static void HelpMarker(const char* desc)
{
	ImGui::TextDisabled("(?)");
	if (ImGui::IsItemHovered())
	{
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::TextUnformatted(desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

void ImguiComponentInspectorWindow::applyFilters(ImguiDebugData& debugData)
{
	 mPropertyFiltersWidget.getFilteredEntities(debugData, mFilteredEntities);
}

void ImguiComponentInspectorWindow::showEntityId()
{
	if (mSelectedEntity.isValid())
	{
		ImGui::Text("id:%llu", static_cast<unsigned long long>(mSelectedEntity.getEntity().getRawId()));
		ImGui::SameLine();
		if (ImGui::Button("Copy"))
		{
			ImGui::LogToClipboard();
			ImGui::LogText("id:%llu", static_cast<unsigned long long>(mSelectedEntity.getEntity().getRawId()));
			ImGui::LogFinish();
		}
	}
}

void ImguiComponentInspectorWindow::showFilteredEntities()
{
	if (!mFilteredEntities.empty())
	{
		if (ImGui::TreeNode("Filtered entities"))
		{
			ImGui::BeginGroup();
			auto scrollBoxSize = ImVec2(200.0f, std::min(180.0f, static_cast<float>(mFilteredEntities.size()) * 17.0f + ImGui::GetStyle().FramePadding.y*4));
			if (ImGui::BeginChild("FilteredEntities", scrollBoxSize, true))
			{
				for (Entity filteredEntity : mFilteredEntities)
				{
					char buf[32];
					sprintf(buf, "id:%llu", static_cast<unsigned long long>(filteredEntity.getRawId()));
					if (ImGui::Selectable(buf, mSelectedEntity.isValid() && mSelectedEntity.getEntity() == filteredEntity))
					{
						mSelectedEntity = filteredEntity;
						std::strcpy(mEntityFilterBuffer, buf);
					}
				}
			}
			ImGui::EndChild();
			ImGui::EndGroup();
			ImGui::TreePop();
		}
	}
}

void ImguiComponentInspectorWindow::showComponentsInspector(ImguiDebugData& debugData)
{
	bool hasFoundAnything = false;
	if (mSelectedEntity.isValid())
	{
		Entity entity = mSelectedEntity.getEntity();

		std::vector<TypedComponent> components;
		debugData.worldHolder.getDynamicWorldLayer().getEntityManager().getAllEntityComponents(entity, components);
		std::ranges::sort(components, [](const auto& a, const auto& b)
		{
			return a.typeId < b.typeId;
		});

		for (TypedComponent componentData : components)
		{
			std::string name = FormatString("%s##ComponentInspection", ID_TO_STR(componentData.typeId).c_str());
			if (ImGui::TreeNode(name.c_str()))
			{
				auto it = mComponentInspectWidgets.find(componentData.typeId);
				if (it != mComponentInspectWidgets.end())
				{
					it->second->update(componentData.component);
				}

				ImGui::TreePop();
				ImGui::Separator();
			}
			hasFoundAnything = true;
		}
	}

	if (!hasFoundAnything && mSelectedEntity.isValid())
	{
		ImGui::Text("No inspectable entity with such ID found");
		ImGui::SameLine(); HelpMarker("An entity without any components also can't be inspectable");
	}
}

void ImguiComponentInspectorWindow::update(ImguiDebugData& debugData)
{
	if (isVisible)
	{
		ImGui::Begin("Component Inspector", &isVisible);

		mPropertyFiltersWidget.update(debugData);

		applyFilters(debugData);

		ImGui::Text("Entities matching the filter: %lu", mFilteredEntities.size());

		showFilteredEntities();

		showEntityId();

		showComponentsInspector(debugData);

		ImGui::End();
	}
}
