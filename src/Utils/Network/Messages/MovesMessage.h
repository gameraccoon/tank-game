#pragma once

#include "Base/Types/BasicTypes.h"
#include "Base/Types/TemplateAliases.h"

#include "GameData/EcsDefinitions.h"

#include "HAL/Network/ConnectionManager.h"

class MovementComponent;
class TransformComponent;
class World;

namespace Network
{
	HAL::ConnectionManager::Message CreateMovesMessage(const TupleVector<Entity, const MovementComponent*, const TransformComponent*>& components, u32 updateIdx);
	void ApplyMovesMessage(World& world, HAL::ConnectionManager::Message&& message);
}
