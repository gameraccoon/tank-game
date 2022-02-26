#pragma once

#include <any>
#include <optional>
#include <vector>
#include <memory>

#include "GameData/EcsDefinitions.h"
#include "GameData/Debug/SubstringSearcher.h"

#include "GameLogic/Imgui/ComponentInspector/PropertyFilters/AbstractPropertyFilter.h"

class WorldCell;
struct ImguiDebugData;

namespace ImguiPropertyFiltration
{
	class AbstractPropertyDescriptor;
	class AbstractPropertyFilterFactory;

	class ImguiPropertyFiltersWidget
	{
	public:
		~ImguiPropertyFiltersWidget();

		void update(ImguiDebugData& debugData);

		void getFilteredEntities(ImguiDebugData& debugData, std::vector<Entity>& inOutEntities);

	private:
		[[nodiscard]] std::vector<StringId> getFilteredComponentTypes() const;
		void init(ImguiDebugData& debugData);

	private:
		static constexpr int MinimalSearchLen = 2;

		char mFilterQueryBuffer[128] = "";
		SubstringSearcher<std::shared_ptr<AbstractPropertyDescriptor>> mPropertyDescriptors;
		std::vector<std::unique_ptr<AbstractPropertyFilter>> mAppliedFilters;
		std::vector<std::shared_ptr<AbstractPropertyFilterFactory>> mSuggestedFiltersFactories;
		std::vector<std::shared_ptr<AbstractPropertyDescriptor>> mLastMatchedProperties;
		OptionalEntity mExplicitlySetEntity;
		bool mIsInited = false;
	};
} // namespace ImguiPropertyFiltration
