#pragma once

#include <algorithm>

#include "Base/Types/TemplateAliases.h"
#include "Base/Debug/Assert.h"

#include "GameLogic/Imgui/ComponentInspector/PropertyFilters/AbstractPropertyDescriptor.h"

namespace ImguiPropertyFiltration
{
	class AbstractPropertyFilter
	{
	public:
		explicit AbstractPropertyFilter(const std::weak_ptr<AbstractPropertyDescriptor>& descriptor)
			: mDescriptor(descriptor)
		{}

		virtual ~AbstractPropertyFilter() = default;

		[[nodiscard]] StringId getComponentType() const
		{
			return getDescriptor()->getComponentType();
		}

		void filterEntities(std::vector<Entity>& inOutEntities, EntityManager& entityManager)
		{
			std::erase_if(
				inOutEntities,
				[this, &entityManager](Entity entity)
				{
					return !isConditionPassed(entityManager, entity);
				}
			);
		}

		[[nodiscard]] virtual std::string getName() const = 0;
		virtual void updateImguiWidget() = 0;

	protected:
		virtual bool isConditionPassed(EntityManager& manager, Entity entity) const = 0;

		[[nodiscard]] std::shared_ptr<AbstractPropertyDescriptor> getDescriptor() const
		{
			std::shared_ptr<AbstractPropertyDescriptor> descriptor = mDescriptor.lock();
			AssertFatal(descriptor, "Property descriptor should exist for PropertyFilter");
			return descriptor;
		}

	private:
		std::weak_ptr<AbstractPropertyDescriptor> mDescriptor;
	};

	class AbstractPropertyFilterFactory
	{
	public:
		virtual ~AbstractPropertyFilterFactory() = default;
		[[nodiscard]] virtual std::string getName() const = 0;
		[[nodiscard]] virtual std::unique_ptr<AbstractPropertyFilter> createFilter() const = 0;
	};

	template<typename T>
	class PropertyFilterFactory : public AbstractPropertyFilterFactory
	{
	public:
		explicit PropertyFilterFactory(const std::weak_ptr<AbstractPropertyDescriptor>& descriptor)
			: mDescriptor(descriptor)
		{}

		[[nodiscard]] std::string getName() const final { return T::GetStaticName(); }

		[[nodiscard]] std::unique_ptr<AbstractPropertyFilter> createFilter() const final
		{
			return std::make_unique<T>(mDescriptor);
		}

	private:
		std::weak_ptr<AbstractPropertyDescriptor> mDescriptor;
	};

} // namespace ImguiPropertyFiltration
