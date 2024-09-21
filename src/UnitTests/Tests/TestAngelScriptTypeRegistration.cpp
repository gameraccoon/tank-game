#include "EngineCommon/precomp.h"

#include <filesystem>

#include <angelscript.h>
#include <autowrapper/aswrappedcall.h>
#include <gtest/gtest.h>
#include <scriptbuilder/scriptbuilder.h>

#include "GameUtils/Scripting/AngelScriptContext.h"
#include "GameUtils/Scripting/AngelScriptEnumRegistration.h"
#include "GameUtils/Scripting/AngelScriptGlobalEngine.h"

#include "UnitTests/TestAssertHelper.h"

TEST(AngelScriptTypeRegistration, Enum_Registered_CanBeInstanciatedInScript)
{
	enum class TestEnum
	{
		FirstValue,
		SecondValue
	};

	AngelScriptGlobalEngine engine;

	AngelScript::RegisterEnum(engine.GetEngine(), "TestEnum");
	AngelScript::RegisterEnumValue(engine.GetEngine(), "TestEnum", "FirstValue", static_cast<int>(TestEnum::FirstValue));
	AngelScript::RegisterEnumValue(engine.GetEngine(), "TestEnum", "SecondValue", static_cast<int>(TestEnum::SecondValue));

	CScriptBuilder builder;
	int r = builder.StartNewModule(&AngelScriptGlobalEngine::GetEngine(), "MyModule");
	if (r < 0)
	{
		ReportFatalError("Unrecoverable error while starting a new module. Possibly out of memory.");
		return;
	}
	r = builder.AddSectionFromMemory("MySection", "void main() { TestEnum testEnum = TestEnum::SecondValue; if (testEnum != 1) { throw(\"Enum value is not correct\"); } }");
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

	// find the function that is to be called
	const asIScriptModule* mod = AngelScriptGlobalEngine::GetEngine().GetModule("MyModule");
	asIScriptFunction* func = mod->GetFunctionByDecl("void main()");
	if (func == nullptr)
	{
		ReportError("The script must have the function 'void main()'. Please add it and try again.");
		return;
	}

	AngelScriptContext ctx;

	ctx.ExecuteFunction(func);
}
