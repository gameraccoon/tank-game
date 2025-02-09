#include "EngineCommon/precomp.h"

#include "GameUtils/Scripting/AngelScriptEnumRegistration.h"

#include <angelscript.h>

#include "GameData/LogCategories.h"

namespace AngelScript
{
	void RegisterEnum(asIScriptEngine& scriptEngine, const char* enumName) noexcept
	{
		const int resCode = scriptEngine.RegisterEnum(enumName);
		if (resCode < 0) [[unlikely]]
		{
			const char* errorReason = "Unknown";
			switch (resCode)
			{
			case asINVALID_NAME:
				errorReason = "The name is null, not an identifier, or it is a reserved keyword";
				break;
			case asALREADY_REGISTERED:
				errorReason = "Another type with this name already exists";
				break;
			case asERROR:
				errorReason = "The name couldn't be parsed";
				break;
			case asNAME_TAKEN:
				errorReason = "The type name is already taken";
				break;
			default:
				ReportErrorRelease("Unhandled error code %d during registration of enum '%s'", resCode, enumName);
				break;
			}
			LogError(LOG_ANGELSCRIPT, "Failed to register enum '%s'. Error code: %d, Error reason: '%s'", enumName, resCode, errorReason);
		}
	}

	void RegisterEnumValue(asIScriptEngine& scriptEngine, const char* enumName, const char* enumValue, const int value) noexcept
	{
		const int resCode = scriptEngine.RegisterEnumValue(enumName, enumValue, value);
		if (resCode < 0) [[unlikely]]
		{
			const char* errorReason = "Unknown";
			switch (resCode)
			{
			case asWRONG_CONFIG_GROUP:
				errorReason = "The enum type was registered in a different configuration group";
				break;
			case asINVALID_TYPE:
				errorReason = "The enum name is invalid";
				break;
			case asALREADY_REGISTERED:
				errorReason = "The enum value is already registered for this enum";
				break;
			default:
				ReportErrorRelease("Unhandled error code %d during registration of enum value '%s' for enum '%s'", resCode, enumValue, enumName);
			}
			LogError(LOG_ANGELSCRIPT, "Failed to register enum value '%s' for enum '%s'. Error code: %d, Error reason: '%s'", enumValue, enumName, resCode, errorReason);
		}
	}
} // namespace AngelScript
