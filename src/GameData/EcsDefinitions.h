#pragma once

#include "EngineCommon/Types/String/StringId.h"

#include <raccoon-ecs/entity_manager.h>
#include <raccoon-ecs/component_set_holder.h>
#include <raccoon-ecs/component_factory.h>
#include <raccoon-ecs/typed_component.h>
#include <raccoon-ecs/entity.h>
#include <raccoon-ecs/entity_manager.h>
#include <raccoon-ecs/utils/entity_view.h>
#include <raccoon-ecs/utils/combined_entity_manager_view.h>

using EntityManager = RaccoonEcs::EntityManagerImpl<StringId>;
using EntityView = RaccoonEcs::EntityViewImpl<EntityManager>;
using ComponentSetHolder = RaccoonEcs::ComponentSetHolderImpl<StringId>;
using ComponentFactory = RaccoonEcs::ComponentFactoryImpl<StringId>;
using TypedComponent = RaccoonEcs::TypedComponentImpl<StringId>;
using ConstTypedComponent = RaccoonEcs::ConstTypedComponentImpl<StringId>;
using Entity = RaccoonEcs::Entity;
using OptionalEntity = RaccoonEcs::OptionalEntity;
using CombinedEntityManagerView = RaccoonEcs::CombinedEntityManagerView<EntityManager, class WorldLayer*>;
