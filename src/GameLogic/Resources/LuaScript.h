#pragma once

#include <optional>

#include "EngineCommon/Types/String/ResourcePath.h"

#include "EngineData/Resources/Resource.h"

#include "GameUtils/Scripting/LuaInstance.h"

namespace Scripting
{
	// Loads a script from a text file, applies it to the lua instance and stores the lua instance for later use
	class LuaScript final : public Resource
	{
	public:
		LuaScript() = default;
		explicit LuaScript(LuaInstance&& luaInstance);

		bool isValid() const override;
		LuaInstance* getLuaInstance() const { return mLuaInstancePtr; }

		static std::string GetUniqueId(const RelativeResourcePath& filename);
		static InitSteps GetInitSteps();
		DeinitSteps getDeinitSteps() const override;

	private:
		std::optional<LuaInstance> mLuaInstance;
		LuaInstance* mLuaInstancePtr = nullptr;
	};
} // namespace Scripting
