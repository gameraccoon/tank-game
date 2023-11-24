#pragma once

#include "Base/Types/String/ResourcePath.h"

#include "Utils/Multithreading/ThreadPool.h"
#include "Utils/ResourceManagement/ResourceManager.h"

#include "HAL/Base/Engine.h"

#include "GameLogic/Render/RenderThreadManager.h"

class ApplicationData
{
public:
		enum class Render
		{
			Enabled,
			Disabled,
		};
public:
	static constexpr int DefaultWorkerThreadCount = 3;

	// we always have one main thread (doesn't matter if we're headless client or dedicated server)
	const int MainThreadId = 0;
	// we always have a resource loading thread
	const int ResourceLoadingThreadId = 1;
	// we always have one or more worker threads
	const int WorkerThreadsCount = DefaultWorkerThreadCount;
	// we may have extra threads (e.g. if we run server and one or more clients all together)
	const int ExtraThreadsCount = 0;
	// we may have render thread (don't have it on dedicated server)
	const int RenderThreadId = -1;
	std::string ScopedProfileOutputPath = "./scoped_profile.json";

	ThreadPool threadPool;
	ResourceManager resourceManager;
#ifndef DEDICATED_SERVER
	RenderThreadManager renderThread;
	std::optional<HAL::Engine> engine;
#endif // !DEDICATED_SERVER

	bool renderEnabled = true;

public:
	explicit ApplicationData(int workerThreadsCount, int extraThreadsCount, const std::filesystem::path& executableFolderPath, Render render = Render::Enabled);

#ifndef DEDICATED_SERVER
	void startRenderThread();
#endif // !DEDICATED_SERVER
	void writeProfilingData();
	void threadSaveProfileData(size_t threadIndex);
	void shutdownThreads();

	int getAdditionalThreadIdByIndex(int additionalThreadIndex) const;

private:
#ifdef ENABLE_SCOPED_PROFILER
	std::vector<std::pair<size_t, ScopedProfilerThreadData::Records>> mScopedProfileRecords;
	std::mutex mScopedProfileRecordsMutex;
#endif // ENABLE_SCOPED_PROFILER
};
