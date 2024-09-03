#pragma once

#define LUA_VALIDATE_IS_TABLE(state, index) \
	if (!LuaInternal::IsTable(state, index)) \
	{ \
		LuaInternal::ReportScriptError(state, "The value is not a table"); \
		return std::nullopt; \
	}

#define LUA_READ_FIELD_INTO_RESULT(state, type, field) \
	if (std::optional<type> field = LuaType::ReadField<type>((state), #field)) \
	{ \
		result.field = std::move(*field); \
	} \
	else \
	{ \
		return std::nullopt; \
	}

#define LUA_READ_FIELD_INTO_VARIABLE(state, type, field, variable) \
	if (std::optional<type> field = LuaType::ReadField<type>((state), #field)) \
	{ \
		variable = std::move(*field); \
	} \
	else \
	{ \
		return std::nullopt; \
	}

#define LUA_READ_FIELD_INTO_SETTER(state, type, field, setter) \
	if (std::optional<type> field = LuaType::ReadField<type>((state), #field)) \
	{ \
		setter(std::move(*field)); \
	} \
	else \
	{ \
		return std::nullopt; \
	}

#define LUA_READ_VALUE_INTO_VARIABLE(state, index, type, variable) \
	type variable; \
	if (std::optional<type> tempVar = LuaType::ReadValue<type>((state), (index))) \
	{ \
		variable = std::move(*tempVar); \
	} \
	else \
	{ \
		return std::nullopt; \
	}
