#pragma once

class asIScriptEngine;

namespace AngelScript
{
	void RegisterEnum(asIScriptEngine& scriptEngine, const char* enumName) noexcept;
	void RegisterEnumValue(asIScriptEngine& scriptEngine, const char* enumName, const char* enumValue, int value) noexcept;
}
