#pragma once

#include "AutoTests/BaseTestCase.h"

class ArgumentsParser;
class TankServerGame;
class TankClientGame;

class BaseNetworkingTestCase : public BaseTestCase
{
public:
	explicit BaseNetworkingTestCase(int clientsCount);
	~BaseNetworkingTestCase() override;

	TestChecklist start(const ArgumentsParser& arguments) override;

protected:
	// override to prepare and return check list that the test will be checking
	virtual TestChecklist prepareChecklist() = 0;
	// override to inject test systems or do other preparations
	virtual void prepareServerGame(TankServerGame& serverGame, const ArgumentsParser& arguments) = 0;
	virtual void prepareClientGame(TankClientGame& clientGame, const ArgumentsParser& arguments, size_t clientIndex) = 0;

	// override to execute the contents of the update loop
	virtual void updateLoop() = 0;

	// override if we should have an external stop condition that is not part of the test checklist
	virtual bool shouldStop() const { return false; }

	// override if you want to override the arguments for the server or client
	virtual ArgumentsParser overrideServerArguments(const ArgumentsParser& arguments);
	virtual ArgumentsParser overrideClientArguments(const ArgumentsParser& arguments, size_t clientIndex);

	void updateServer();
	void updateClient(int clientIndex);

	int getClientCount() const { return mClientsCount; }

private:
	const int mClientsCount;

	std::unique_ptr<TankServerGame> mServerGame;
	std::vector<std::unique_ptr<TankClientGame>> mClientGames;
};
