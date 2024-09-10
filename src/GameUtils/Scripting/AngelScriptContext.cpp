#include "EngineCommon/precomp.h"

#include "GameUtils/Scripting/AngelScriptContext.h"

#include <angelscript.h>

#include "GameUtils/Scripting/AngelScriptGlobalEngine.h"

AngelScriptContext::AngelScriptContext()
{
	// Create our context, prepare it, and then execute
	mContext = AngelScriptGlobalEngine::GetEngine().CreateContext();
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
		// The execution didn't complete as expected. Determine what happened.
		if (resCode == asEXECUTION_EXCEPTION)
		{
			// An exception occurred, let the script writer know what happened so it can be corrected.
			ReportError("An exception '%s' occurred. Please correct the code and try again.", mContext->GetExceptionString());
		}
	}
}
