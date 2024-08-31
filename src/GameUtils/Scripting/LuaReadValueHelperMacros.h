#pragma once

#define LUA_VALIDATE_IS_TABLE(state, index) \
	if (!LuaInternal::IsTable(state, index)) \
	{ \
		LuaInternal::ReportScriptError(state, "The value is not a table"); \
		return std::nullopt; \
	}

#define LUA_READ_FIELD_INTO_RESULT(state, type, field) \
	if (const std::optional<type> field = ReadField<type>(state, #field)) \
	{ \
		result.field = *field; \
	} \
	else \
	{ \
		return std::nullopt; \
	}

#define LUA_READ_FIELD_INTO_VARIABLE(state, type, field, variable) \
	if (const std::optional<type> field = ReadField<type>(state, #field)) \
	{ \
		variable = *field; \
	} \
	else \
	{ \
		return std::nullopt; \
	}
