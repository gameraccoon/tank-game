#include "EngineCommon/precomp.h"

#include "GameUtils/Scripting/AngelScriptObjectTypeRegistration.h"

#include <angelscript.h>

#include "GameData/LogCategories.h"

namespace AngelScript
{
	void RegisterObjectType(asIScriptEngine& scriptEngine, const char* objectTypeName, const int sizeBytes, const uint64_t flags) noexcept
	{
		const int resCode = scriptEngine.RegisterObjectType(objectTypeName, sizeBytes, flags);
		if (resCode < 0) [[unlikely]]
		{
			const char* errorReason = "Unknown";
			switch (resCode)
			{
			case asINVALID_ARG:
				errorReason = "The flags are invalid";
				break;
			case asINVALID_NAME:
				errorReason = "The name is invalid";
				break;
			case asALREADY_REGISTERED:
				errorReason = "Another type of the same name already exists";
				break;
			case asNAME_TAKEN:
				errorReason = "The name conflicts with other symbol names";
				break;
			case asLOWER_ARRAY_DIMENSION_NOT_REGISTERED:
				errorReason = "When registering an array type the array element must be a primitive or a registered type";
				break;
			case asINVALID_TYPE:
				errorReason = "The object type was not properly formed";
				break;
			case asNOT_SUPPORTED:
				errorReason = "The object type is not supported, or already in use preventing it from being overloaded";
				break;
			default:
				ReportErrorRelease("Unhandled error code %d during registration of object type '%s'", resCode, objectTypeName);
				break;
			}
			LogError(LOG_ANGELSCRIPT, "Failed to register object type '%s'. Error code: %d, Error reason: '%s'", objectTypeName, resCode, errorReason);
		}
	}

	void RegisterObjectProperty(asIScriptEngine& scriptEngine, const char* objectTypeName, const char* propertyDeclaration, const int byteOffset) noexcept
	{
		const int resCode = scriptEngine.RegisterObjectProperty(objectTypeName, propertyDeclaration, byteOffset);
		if (resCode < 0) [[unlikely]]
		{
			const char* errorReason = "Unknown";
			switch (resCode)
			{
			case asWRONG_CONFIG_GROUP:
				errorReason = "The object type was registered in a different configuration group";
				break;
			case asINVALID_OBJECT:
				errorReason = "The object type name does not specify an object type";
				break;
			case asINVALID_TYPE:
				errorReason = "The object type name has invalid syntax";
				break;
			case asINVALID_DECLARATION:
				errorReason = "The property declaration is invalid";
				break;
			case asNAME_TAKEN:
				errorReason = "The property name conflicts with other members";
				break;
			default:
				ReportErrorRelease("Unhandled error code %d during registration of object property '%s' of type '%s", resCode, propertyDeclaration, objectTypeName);
				break;
			}
			LogError(LOG_ANGELSCRIPT, "Failed to register object property '%s' of type '%s'. Error code: %d, Error reason: '%s'", propertyDeclaration, objectTypeName, resCode, errorReason);
		}
	}

	void RegisterObjectMethod(asIScriptEngine& scriptEngine, const char* objectTypeName, const char* methodDeclaration, const asSFuncPtr& funcPointer) noexcept
	{
		const int resCode = scriptEngine.RegisterObjectMethod(objectTypeName, methodDeclaration, funcPointer, asCALL_GENERIC);
		if (resCode < 0) [[unlikely]]
		{
			const char* errorReason = "Unknown";
			switch (resCode)
			{
			case asWRONG_CONFIG_GROUP:
				errorReason = "The object type was registered in a different configuration group";
				break;
			case asNOT_SUPPORTED:
				errorReason = "The calling convention is not supported";
				break;
			case asINVALID_TYPE:
				errorReason = "The object type name is not a valid object name";
				break;
			case asINVALID_DECLARATION:
				errorReason = "The declaration is invalid";
				break;
			case asNAME_TAKEN:
				errorReason = "The name conflicts with other members";
				break;
			case asWRONG_CALLING_CONV:
				errorReason = "The function's calling convention isn't compatible with callConv";
				break;
			case asALREADY_REGISTERED:
				errorReason = "The method has already been registered with the same parameter list";
				break;
			case asINVALID_ARG:
				errorReason = "	The auxiliary pointer wasn't set according to calling convention";
				break;
			default:
				ReportErrorRelease("Unhandled error code %d during registration of object method '%s' of type '%s", resCode, methodDeclaration, objectTypeName);
				break;
			}
			LogError(LOG_ANGELSCRIPT, "Failed to register object method '%s' of type '%s'. Error code: %d, Error reason: '%s'", methodDeclaration, objectTypeName, resCode, errorReason);
		}
	}
} // namespace AngelScript
