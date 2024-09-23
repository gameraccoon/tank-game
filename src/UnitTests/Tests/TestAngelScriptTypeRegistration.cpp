#include "EngineCommon/precomp.h"

#include <filesystem>

#include <angelscript.h>
#include <autowrapper/aswrappedcall.h>
#include <gtest/gtest.h>
#include <scriptbuilder/scriptbuilder.h>

#include "GameUtils/Scripting/AngelScriptContext.h"
#include "GameUtils/Scripting/AngelScriptEnumRegistration.h"
#include "GameUtils/Scripting/AngelScriptGlobalEngine.h"
#include "GameUtils/Scripting/AngelScriptObjectTypeRegistration.h"

#include "UnitTests/TestAssertHelper.h"

namespace AngelScriptTypeRegistrationInternal
{
	static void RunScriptAndCallMain(const char* script)
	{
		CScriptBuilder builder;
		int r = builder.StartNewModule(&AngelScriptGlobalEngine::GetEngine(), "MyModule");
		if (r < 0)
		{
			ReportFatalError("Unrecoverable error while starting a new module. Possibly out of memory.");
			return;
		}
		r = builder.AddSectionFromMemory("MySection", script);
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
} // namespace AngelScriptTypeRegistrationInternal

TEST(AngelScriptTypeRegistration, Enum_Registered_CanBeInstanciatedInScript)
{
	using namespace AngelScriptTypeRegistrationInternal;

	enum class TestEnum
	{
		FirstValue,
		SecondValue
	};

	AngelScriptGlobalEngine engine;

	AngelScript::RegisterEnum(engine.GetEngine(), "TestEnum");
	AngelScript::RegisterEnumValue(engine.GetEngine(), "TestEnum", "FirstValue", static_cast<int>(TestEnum::FirstValue));
	AngelScript::RegisterEnumValue(engine.GetEngine(), "TestEnum", "SecondValue", static_cast<int>(TestEnum::SecondValue));

	RunScriptAndCallMain("void main() { TestEnum testEnum = TestEnum::SecondValue; if (testEnum != 1) { throw(\"Enum value is not correct\"); } }");
}

TEST(AngelScriptTypeRegistration, ObjectType_Registered_CanBeInstanciatedInScript)
{
	using namespace AngelScriptTypeRegistrationInternal;

	struct TestObject
	{
		int value = 0;
		void TestFunction() const
		{
			ASSERT_EQ(value, 42);
		}
	};

	AngelScriptGlobalEngine engine;

	AngelScript::RegisterObjectType(engine.GetEngine(), "TestObject", sizeof(TestObject), asOBJ_VALUE | asOBJ_POD);
	AngelScript::RegisterObjectProperty(engine.GetEngine(), "TestObject", "int value", offsetof(TestObject, value));
	AngelScript::RegisterObjectMethod(engine.GetEngine(), "TestObject", "void TestFunction()", WRAP_MFN(TestObject, TestFunction));

	RunScriptAndCallMain("void main() { TestObject testObject; testObject.value = 42; testObject.TestFunction(); }");
}
