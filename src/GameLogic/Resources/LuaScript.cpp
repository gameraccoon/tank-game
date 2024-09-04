#include "EngineCommon/precomp.h"

#include "GameLogic/Resources/LuaScript.h"

#include <filesystem>

#include <nlohmann/json.hpp>

#include "EngineCommon/Types/String/ResourcePath.h"

#include "EngineUtils/ResourceManagement/ResourceManager.h"

namespace Scripting
{
	static UniqueAny ExecuteScriptFile(UniqueAny&& resource, ResourceManager& resourceManager, const ResourceHandle /*handle*/)
	{
		SCOPED_PROFILER("ExecuteScriptFile");

		const RelativeResourcePath* pathPtr = resource.cast<RelativeResourcePath>();

		if (!pathPtr)
		{
			ReportFatalError("We got an incorrect type of value when loading a Lua script in ExecuteScriptFile (expected RelativeResourcePath)");
			return {};
		}

		const AbsoluteResourcePath& path = resourceManager.getAbsoluteResourcePath(*pathPtr);

		LuaInstance instance(LuaInstance::OpenStandardLibs::Yes);
		const LuaExecResult execResult = instance.execScriptFromFile(path);
		if (execResult.statusCode != 0) [[unlikely]]
		{
			ReportError("Failed to execute lua script '%s': %s", path.getAbsolutePath().c_str(), execResult.errorMessage.c_str());
			return {};
		}

		return UniqueAny::Create<LuaScript::Ptr>(std::make_unique<LuaScript>(std::move(instance)));
	}

	LuaScript::LuaScript(LuaInstance&& luaInstance)
		: mLuaInstance(std::move(luaInstance))
		, mLuaInstancePtr(&mLuaInstance.value())
	{
	}

	bool LuaScript::isValid() const
	{
		return mLuaInstance.has_value();
	}

	std::string LuaScript::GetUniqueId(const RelativeResourcePath& filename)
	{
		return filename.getRelativePathStr();
	}

	Resource::InitSteps LuaScript::GetInitSteps()
	{
		return {
			InitStep{
				.thread = Thread::Loading,
				.init = &ExecuteScriptFile,
			},
		};
	}

	Resource::DeinitSteps LuaScript::getDeinitSteps() const
	{
		return {};
	}
} // namespace Scripting
