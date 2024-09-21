#include "EngineCommon/precomp.h"

#include "GameUtils/Scripting/AngelScriptGlobalEngine.h"

#include <angelscript.h>
#include <scripthandle/scripthandle.h>
#include <scriptstdstring/scriptstdstring.h>
#include <weakref/weakref.h>

class AngelScriptGlobalEngine::Impl
{
public:
	Impl()
	{
		// sanity check that we didn't accidentally change the version without verifying it
		constexpr asDWORD expectedVersion = 23700;
		Assert(ANGELSCRIPT_VERSION == expectedVersion, "AngelScript has unexpected version, expected %u, got %u", expectedVersion, ANGELSCRIPT_VERSION);

		mEngine = asCreateScriptEngine(expectedVersion);
		if (!mEngine)
		{
			ReportFatalError("Failed to create AngelScript engine");
		}

		const int resCode = mEngine->SetMessageCallback(asFUNCTION(MessageCallback), nullptr, asCALL_CDECL);
		if (resCode < 0)
		{
			LogError("Failed to set message callback for AngelScript engine. Error code: %d", resCode);
		}

		// register std::string as the string type
		RegisterStdString(mEngine);
		// register a custom generic handle type (can be changed to a different type if needed)
		RegisterScriptHandle(mEngine);
		// register weakref and const_weakref types
		RegisterScriptWeakRef(mEngine);
	}

	~Impl()
	{
		const int resCode = mEngine->ShutDownAndRelease();
		if (resCode < 0)
		{
			LogError("Failed to shutdown AngelScript engine. Error code: %d", resCode);
		}
	}

	static inline asIScriptEngine* mEngine = nullptr;

private:
	static void MessageCallback(const asSMessageInfo* msg, void* /*param*/)
	{
		if (msg->type == asMSGTYPE_WARNING)
		{
			LogWarning("[Script]: %s (%d, %d) : %s", msg->section, msg->row, msg->col, msg->message);
		}
		else if (msg->type == asMSGTYPE_ERROR)
		{
			LogError("[Script]: %s (%d, %d) : %s", msg->section, msg->row, msg->col, msg->message);
		}
		else
		{
			LogInfo("[Script]: %s (%d, %d) : %s", msg->section, msg->row, msg->col, msg->message);
		}
	}
};

AngelScriptGlobalEngine::AngelScriptGlobalEngine()
	: mPimpl(std::make_unique<Impl>())
{
}

AngelScriptGlobalEngine::~AngelScriptGlobalEngine() = default;

asIScriptEngine& AngelScriptGlobalEngine::GetEngine()
{
	AssertFatal(Impl::mEngine, "AngelScript engine is not initialized. Going to crash now.");
	return *Impl::mEngine;
}
