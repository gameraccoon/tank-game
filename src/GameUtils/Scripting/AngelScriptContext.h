#pragma once

class asIScriptContext;
class asIScriptFunction;

class AngelScriptContext
{
public:
	AngelScriptContext();
	~AngelScriptContext();

	void ExecuteFunction(asIScriptFunction* func);

private:
	asIScriptContext* mContext = nullptr;
};
