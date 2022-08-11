#include "Base/precomp.h"

#include "GameLogic/Game/ApplicationData.h"

#ifdef ENABLE_SCOPED_PROFILER
#include "Utils/Profiling/ProfileDataWriter.h"
#endif // ENABLE_SCOPED_PROFILER

#include "GameLogic/Game/TankServerGame.h"
#include "GameLogic/Render/RenderAccessor.h"

ApplicationData::ApplicationData(int threadsCount)
	: WorkerThreadsCount(threadsCount)
	, RenderThreadId(threadsCount + 1)
	, ResourceLoadingThreadId(threadsCount + 2)
	, ServerThreadId(threadsCount + 3)
	, threadPool(threadsCount, [this]{ threadSaveProfileData(ThreadPool::GetThisThreadId()); })
{
	resourceManager.startLoadingThread([this]{ threadSaveProfileData(ResourceLoadingThreadId); });
}

#ifndef DEDICATED_SERVER
void ApplicationData::startRenderThread()
{
	engine.releaseRenderContext();
	renderThread.startThread(resourceManager, engine, [&engine = this->engine]{ engine.acquireRenderContext(); });
}
#endif // !DEDICATED_SERVER

void ApplicationData::writeProfilingData()
{
#ifdef ENABLE_SCOPED_PROFILER
	{
		ProfileDataWriter::ProfileData data;
#ifndef DEDICATED_SERVER
		{
			data.scopedProfilerDatas.emplace_back();
			ProfileDataWriter::ScopedProfilerData& renderScopedProfilerData = data.scopedProfilerDatas.back();
			renderScopedProfilerData.records = renderThread.getAccessor().consumeScopedProfilerRecordsUnsafe();
			renderScopedProfilerData.threadId = RenderThreadId;
		}
#endif // !DEDICATED_SERVER
		{
			data.scopedProfilerDatas.emplace_back();
			ProfileDataWriter::ScopedProfilerData& mainScopedProfilerData = data.scopedProfilerDatas.back();
			mainScopedProfilerData.records = gtlScopedProfilerData.getAllRecords();
			mainScopedProfilerData.threadId = MainThreadId;
		}

		for (auto&& [threadId, records] : mScopedProfileRecords)
		{
			data.scopedProfilerDatas.emplace_back(threadId, std::move(records));
		}

		data.threadNames.resize(ServerThreadId + 1);
		data.threadNames[MainThreadId] = "Main Thread";
		data.threadNames[RenderThreadId] = "Render Thread";
		data.threadNames[ResourceLoadingThreadId] = "Resource Loading Thread";
		data.threadNames[ServerThreadId] = "Server Thread";
		for (int i = 0; i < WorkerThreadsCount; ++i)
		{
			// zero is reserved for main thread
			data.threadNames[1 + i] = std::string("Worker Thread #") + std::to_string(i+1);
		}

		ProfileDataWriter::PrintScopedProfileToFile(ScopedProfileOutputPath, data);
	}
#endif // ENABLE_SCOPED_PROFILER
}

void ApplicationData::threadSaveProfileData([[maybe_unused]] size_t threadIndex)
{
#ifdef ENABLE_SCOPED_PROFILER
	std::lock_guard l(mScopedProfileRecordsMutex);
	mScopedProfileRecords.emplace_back(threadIndex, gtlScopedProfilerData.getAllRecords());
#endif // ENABLE_SCOPED_PROFILER
}

void ApplicationData::shutdownThreads()
{
#ifndef DEDICATED_SERVER
	renderThread.shutdownThread();
#endif // !DEDICATED_SERVER
	threadPool.shutdown();
	resourceManager.stopLoadingThread();
}
