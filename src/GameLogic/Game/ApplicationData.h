#pragma once

#include "HAL/Base/Engine.h"

#include "Utils/Multithreading/ThreadPool.h"

#include "GameLogic/Render/RenderThreadManager.h"

class ArgumentsParser;
class ResourceManager;

class ApplicationData
{
public:
	static constexpr int DefaultWorkerThreadCount = 3;
	const int MainThreadId = 0;
	const int WorkerThreadsCount = DefaultWorkerThreadCount;
	const int ServerThreadId = WorkerThreadsCount + 1;
	const int RenderThreadId = ServerThreadId + 1;
	std::string ScopedProfileOutputPath = "./scoped_profile.json";

	ThreadPool threadPool;
	RenderThreadManager renderThread;

public:
	ApplicationData(int threadsCount);

	void writeProfilingData();
	void workingThreadSaveProfileData();
	void shutdownThreads();

	void serverThreadFunction(ResourceManager& resourceManager, ThreadPool& threadPool, const ArgumentsParser& arguments);

private:
	std::vector<std::pair<size_t, ScopedProfilerThreadData::Records>> mScopedProfileRecords;
	std::mutex mScopedProfileRecordsMutex;
};
