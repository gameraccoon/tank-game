#include "EngineCommon/precomp.h"

#include "GameUtils/Scripting/AngelScriptContext.h"

#include <angelscript.h>

#include "GameData/LogCategories.h"

#include "GameUtils/Scripting/AngelScriptGlobalEngine.h"

AngelScriptContext::AngelScriptContext()
{
	mContext = AngelScriptGlobalEngine::GetEngine().CreateContext();
	AssertFatal(mContext, "Failed to create AngelScript context");

	auto exceptionCallback = [](asIScriptContext* ctx, void* /*obj*/) -> void {
		const asIScriptFunction* function = ctx->GetExceptionFunction();
		const int lineNumber = ctx->GetExceptionLineNumber();

		LogError(
			LOG_ANGELSCRIPT,
			"[ScriptException]: %s\n"
			"function: %s\n"
			"module: %s\n"
			"section: %s\n"
			"line: %d",
			ctx->GetExceptionString(),
			function->GetDeclaration(),
			function->GetModuleName(),
			function->GetScriptSectionName(),
			lineNumber
		);
	};
	using ExceptionCallbackType = void (*)(asIScriptContext*, void*);
	const ExceptionCallbackType exceptionCallbackPtr = exceptionCallback;

	mContext->SetExceptionCallback(asFUNCTION(exceptionCallbackPtr), nullptr, asCALL_CDECL);
}

AngelScriptContext::~AngelScriptContext()
{
	const int resCode = mContext->Release();
	if (resCode != 0)
	{
		ReportError("Failed to release context");
	}
}

void AngelScriptContext::ExecuteFunction(asIScriptFunction* func)
{
	mContext->Prepare(func);
	const int resCode = mContext->Execute();
	if (resCode != asEXECUTION_FINISHED)
	{
		if (resCode == asEXECUTION_EXCEPTION)
		{
			ReportError("An exception '%s' occurred. Please correct the code and try again.", mContext->GetExceptionString());
		}
	}
}
