#pragma once

#include <memory>

class asIScriptEngine;

class AngelScriptGlobalEngine
{
public:
	AngelScriptGlobalEngine();
	AngelScriptGlobalEngine(const AngelScriptGlobalEngine&) = delete;
	AngelScriptGlobalEngine& operator=(const AngelScriptGlobalEngine&) = delete;
	AngelScriptGlobalEngine(AngelScriptGlobalEngine&&) = delete;
	AngelScriptGlobalEngine& operator=(AngelScriptGlobalEngine&&) = delete;
	~AngelScriptGlobalEngine();

	static asIScriptEngine& GetEngine();

private:
	class Impl;
	std::unique_ptr<Impl> mPimpl;
};
