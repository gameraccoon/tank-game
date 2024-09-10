#include "EngineCommon/precomp.h"

#include <filesystem>

#include <angelscript.h>
#include <gtest/gtest.h>
#include <scriptbuilder/scriptbuilder.h>

#include "GameUtils/Scripting/AngelScriptContext.h"
#include "GameUtils/Scripting/AngelScriptGlobalEngine.h"

#include "UnitTests/TestAssertHelper.h"

namespace TestAngelScriptEngineInternal
{
	void print(asIScriptGeneric* scriptGeneric)
	{
		const std::string& message = *static_cast<const std::string*>(scriptGeneric->GetArgAddress(0));
		printf("%s", message.c_str());
	}
}

TEST(AngelScriptEngine, Instance_ExecuteValidScriptFromFile_StatusCodeIsZero)
{
	using namespace TestAngelScriptEngineInternal;

	AngelScriptGlobalEngine engine;

	int r = AngelScriptGlobalEngine::GetEngine().RegisterGlobalFunction("void print(const string& in)", asFUNCTION(print), asCALL_GENERIC);
	Assert(r >= 0, "Failed to register function");

	CScriptBuilder builder;
	r = builder.StartNewModule(&AngelScriptGlobalEngine::GetEngine(), "MyModule");
	if (r < 0)
	{
		// If the code fails here it is usually because there
		// is no more memory to allocate the module
		ReportFatalError("Unrecoverable error while starting a new module.");
		return;
	}
	r = builder.AddSectionFromFile("resources/unittests/AngelScript-ValidScript.as");
	if (r < 0)
	{
		// The builder wasn't able to load the file. Maybe the file
		// has been removed, or the wrong name was given, or some
		// preprocessing commands are incorrectly written.
		ReportError("Please correct the errors in the script and try again.");
		return;
	}
	r = builder.BuildModule();
	if (r < 0)
	{
		// An error occurred. Instruct the script writer to fix the
		// compilation errors that were listed in the output stream.
		ReportError("Please correct the errors in the script and try again.");
		return;
	}

	// Find the function that is to be called.
	const asIScriptModule* mod = AngelScriptGlobalEngine::GetEngine().GetModule("MyModule");
	asIScriptFunction* func = mod->GetFunctionByDecl("void main()");
	if (func == nullptr)
	{
		// The function couldn't be found. Instruct the script writer
		// to include the expected function in the script.
		ReportError("The script must have the function 'void main()'. Please add it and try again.");
		return;
	}

	AngelScriptContext ctx;

	ctx.ExecuteFunction(func);
}

TEST(AngelScriptEngine, Instance_ExecuteInvalidScriptFromFile_StatusCodeIsNotZero)
{
	using namespace TestAngelScriptEngineInternal;

	AngelScriptGlobalEngine engine;

	CScriptBuilder builder;
	int r = builder.StartNewModule(&AngelScriptGlobalEngine::GetEngine(), "MyModule");
	if (r < 0)
	{
		// If the code fails here it is usually because there
		// is no more memory to allocate the module
		ReportFatalError("Unrecoverable error while starting a new module.");
		return;
	}
	r = builder.AddSectionFromFile("resources/unittests/AngelScript-InvalidScript.as");
	if (r < 0)
	{
		// The builder wasn't able to load the file. Maybe the file
		// has been removed, or the wrong name was given, or some
		// preprocessing commands are incorrectly written.
		ReportError("Please correct the errors in the script and try again.");
		return;
	}
	r = builder.BuildModule();
	ASSERT_LT(r, 0);
}

TEST(AngelScriptEngine, Instance_ExecuteNonExistentScriptFromFile_StatusCodeIsNotZero)
{
	using namespace TestAngelScriptEngineInternal;

	AngelScriptGlobalEngine engine;

	CScriptBuilder builder;
	int r = builder.StartNewModule(&AngelScriptGlobalEngine::GetEngine(), "MyModule");
	if (r < 0)
	{
		// If the code fails here it is usually because there
		// is no more memory to allocate the module
		ReportFatalError("Unrecoverable error while starting a new module.");
		return;
	}
	r = builder.AddSectionFromFile("resources/unittests/AngelScript-NonExistentScript.as");
	ASSERT_LT(r, 0);
}
