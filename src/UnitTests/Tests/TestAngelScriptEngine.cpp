#include "EngineCommon/precomp.h"

#include <filesystem>

#include <angelscript.h>
#include <gtest/gtest.h>
#include <scriptbuilder/scriptbuilder.h>

#include "GameUtils/Scripting/AngelScriptContext.h"
#include "GameUtils/Scripting/AngelScriptGlobalEngine.h"

#include "UnitTests/TestAssertHelper.h"

TEST(AngelScriptEngine, Instance_ExecuteValidScriptFromFile_StatusCodeIsZero)
{
	static int callCount = 0;
	const int expectedCallCount = callCount + 1;
	auto testGenericFunction = [](asIScriptGeneric* scriptGeneric) {
		const std::string& message = *static_cast<const std::string*>(scriptGeneric->GetArgAddress(0));
		EXPECT_STREQ("Test text", message.c_str());
		++callCount;
	};
	using GenericFunction = void (*)(asIScriptGeneric*);
	const GenericFunction testGenericFunctionPtr = testGenericFunction;

	AngelScriptGlobalEngine engine;

	int r = AngelScriptGlobalEngine::GetEngine().RegisterGlobalFunction("void testFunction(const string& in)", asFUNCTION(testGenericFunctionPtr), asCALL_GENERIC);
	Assert(r >= 0, "Failed to register function");

	CScriptBuilder builder;
	r = builder.StartNewModule(&AngelScriptGlobalEngine::GetEngine(), "MyModule");
	if (r < 0)
	{
		ReportFatalError("Unrecoverable error while starting a new module. Possibly out of memory.");
		return;
	}
	r = builder.AddSectionFromFile("resources/unittests/AngelScript-ValidScript.as");
	if (r < 0)
	{
		ReportError("Please correct the errors in the script and try again.");
		return;
	}
	r = builder.BuildModule();
	if (r < 0)
	{
		ReportError("Please correct the errors in the script and try again.");
		return;
	}

	// Find the function that is to be called.
	const asIScriptModule* mod = AngelScriptGlobalEngine::GetEngine().GetModule("MyModule");
	asIScriptFunction* func = mod->GetFunctionByDecl("void main()");
	if (func == nullptr)
	{
		ReportError("The script must have the function 'void main()'. Please add it and try again.");
		return;
	}

	AngelScriptContext ctx;

	ctx.ExecuteFunction(func);

	EXPECT_EQ(expectedCallCount, callCount);
}

TEST(AngelScriptEngine, Instance_ExecuteInvalidScriptFromFile_StatusCodeIsNotZero)
{
	AngelScriptGlobalEngine engine;

	CScriptBuilder builder;
	int r = builder.StartNewModule(&AngelScriptGlobalEngine::GetEngine(), "MyModule");
	if (r < 0)
	{
		ReportFatalError("Unrecoverable error while starting a new module. Possibly out of memory.");
		return;
	}
	r = builder.AddSectionFromFile("resources/unittests/AngelScript-InvalidScript.as");
	if (r < 0)
	{
		ReportError("Please correct the errors in the script and try again.");
		return;
	}
	r = builder.BuildModule();
	ASSERT_LT(r, 0);
}

TEST(AngelScriptEngine, Instance_ExecuteNonExistentScriptFromFile_StatusCodeIsNotZero)
{
	AngelScriptGlobalEngine engine;

	CScriptBuilder builder;
	int r = builder.StartNewModule(&AngelScriptGlobalEngine::GetEngine(), "MyModule");
	if (r < 0)
	{
		ReportFatalError("Unrecoverable error while starting a new module. Possibly out of memory.");
		return;
	}
	r = builder.AddSectionFromFile("resources/unittests/AngelScript-NonExistentScript.as");
	ASSERT_LT(r, 0);
}

TEST(AngelScriptEngine, Instance_ExecuteScriptFromFileThatRaisesException_AssertFired)
{
	AngelScriptGlobalEngine engine;

	CScriptBuilder builder;
	int r = builder.StartNewModule(&AngelScriptGlobalEngine::GetEngine(), "MyModule");
	if (r < 0)
	{
		ReportFatalError("Unrecoverable error while starting a new module. Possibly out of memory.");
		return;
	}
	r = builder.AddSectionFromFile("resources/unittests/AngelScript-ScriptRaisingException.as");
	if (r < 0)
	{
		ReportError("Please correct the errors in the script and try again.");
		return;
	}
	r = builder.BuildModule();
	if (r < 0)
	{
		ReportError("Please correct the errors in the script and try again.");
		return;
	}

	// Find the function that is to be called.
	const asIScriptModule* mod = AngelScriptGlobalEngine::GetEngine().GetModule("MyModule");
	asIScriptFunction* func = mod->GetFunctionByDecl("void main()");
	if (func == nullptr)
	{
		ReportError("The script must have the function 'void main()'. Please add it and try again.");
		return;
	}

	AngelScriptContext ctx;

	{
		DisableAssertGuard guard;
		ctx.ExecuteFunction(func);
		EXPECT_EQ(guard.getTriggeredAssertsCount(), 1);
	}
}
