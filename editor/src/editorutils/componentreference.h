#pragma once

#include <optional>

#include "GameData/EcsDefinitions.h"

/**
 * @brief A component can be stored in variety of ways and reference combinations
 * | Referenced by | In ComponentHolder (Unique) | In EntityManager (referenced by Entity) |
 * | (Unique)      | Game Components             |                                         |
 * | WorldID?      | World Components            | World Entities                          |
 *
 * For example if a component is bound to a World entity, then it needs WorldID,
 * and Entity to be referenced properly.
 * In contrary to reference a Game Component nothing is needed (there's only one
 * such component possible).
 */
struct ComponentSourceReference
{
	/** nullopt indicates that the component stored not by EntityManager */
	OptionalEntity entity;
	/** false indicates that the component is not bound to any world */
	bool isWorld = false; // std::optional<WorldID>?
};

struct ComponentReference
{
	ComponentSourceReference source;
	StringId componentTypeName;
};
