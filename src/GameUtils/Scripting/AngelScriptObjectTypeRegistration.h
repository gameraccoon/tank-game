#pragma once

class asIScriptEngine;
struct asSFuncPtr;

namespace AngelScript
{
	void RegisterObjectType(asIScriptEngine& scriptEngine, const char* objectTypeName, int sizeBytes, uint64_t flags) noexcept;
	void RegisterObjectProperty(asIScriptEngine& scriptEngine, const char* objectTypeName, const char* propertyDeclaration, int byteOffset) noexcept;
	void RegisterObjectMethod(asIScriptEngine& scriptEngine, const char* objectTypeName, const char* methodDeclaration, const asSFuncPtr& funcPointer) noexcept;
}
