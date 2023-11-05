#include "Base/precomp.h"

#include "GameLogic/Game/ApplicationData.h"

#ifdef ENABLE_SCOPED_PROFILER
#include "Utils/Profiling/ProfileDataWriter.h"
#endif // ENABLE_SCOPED_PROFILER

#include "GameLogic/Game/TankServerGame.h"
#include "GameLogic/Render/RenderAccessor.h"

ApplicationData::ApplicationData(int workerThreadsCount, int extraThreadsCount, Render render)
	: WorkerThreadsCount(workerThreadsCount)
	, ExtraThreadsCount(extraThreadsCount)
	, RenderThreadId(ResourceLoadingThreadId + 1 + workerThreadsCount + extraThreadsCount)
	, threadPool(workerThreadsCount, [this]{ threadSaveProfileData(ThreadPool::GetThisThreadId()); }, ResourceLoadingThreadId + 1)
	, renderEnabled(render == Render::Enabled)
{
	if (renderEnabled)
	{
		engine.emplace(800, 600);
		resourceManager.startLoadingThread([this] { threadSaveProfileData(ResourceLoadingThreadId); });
	}
}

#ifndef DEDICATED_SERVER
void ApplicationData::startRenderThread()
{
	engine->releaseRenderContext();
	renderThread.startThread(resourceManager, engine.value(), [&engineRef = *engine]{ engineRef.acquireRenderContext(); });
}
#endif // !DEDICATED_SERVER

void ApplicationData::writeProfilingData()
{
#ifdef ENABLE_SCOPED_PROFILER
	{
		ProfileDataWriter::ProfileData data;
#ifndef DEDICATED_SERVER
		if (renderEnabled)
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

		data.threadNames.resize(getAdditionalThreadIdByIndex(ExtraThreadsCount) + 1);
		data.threadNames[MainThreadId] = "Main Thread";
		data.threadNames[ResourceLoadingThreadId] = "Resource Loading Thread";
		for (int i = 0; i < WorkerThreadsCount; ++i)
		{
			// zero is reserved for main thread
			data.threadNames[ResourceLoadingThreadId + 1 + i] = std::string("Worker Thread #") + std::to_string(i+1);
		}
		for (int i = 0; i < ExtraThreadsCount; ++i)
		{
			data.threadNames[getAdditionalThreadIdByIndex(i)] = std::string("Extra Thread #") + std::to_string(i+1);
		}
		if (RenderThreadId != -1)
		{
			data.threadNames[RenderThreadId] = "Render Thread";
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

int ApplicationData::getAdditionalThreadIdByIndex(int additionalThreadIndex) const
{
	return ResourceLoadingThreadId + 1 + WorkerThreadsCount + additionalThreadIndex;
}
