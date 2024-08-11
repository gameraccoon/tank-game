#include "EngineCommon/precomp.h"

#include "GameUtils/ResourceManagement/ResourceStorageData.h"

namespace ResourceLoading
{
	ResourceHandle ResourceStorage::createResourceLock(const std::string& id)
	{
		const ResourceHandle currentHandle(handleIdx);
		idsMap[id] = currentHandle;
		idFindMap[currentHandle] = id;
		resourceLocksCount[currentHandle] = 1;
		++handleIdx;
		return currentHandle;
	}
}
