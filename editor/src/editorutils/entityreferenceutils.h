#pragma once

#include <raccoon-ecs/entity_view.h>

class World;

namespace Utils
{
	using EntityView = RaccoonEcs::EntityViewImpl<StringId>;
	std::optional<EntityView> GetEntityView(RaccoonEcs::Entity entity, World* world);
}
