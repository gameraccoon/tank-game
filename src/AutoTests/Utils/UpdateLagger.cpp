#include "Base/precomp.h"

#include "AutoTests/Utils/UpdateLagger.h"

UpdateLagger::UpdateLagger(std::vector<int>&& framePauses)
	: mFramePauses(framePauses)
{}

bool UpdateLagger::shouldUpdate()
{
	if (mPauseIdx < mFramePauses.size())
	{
		if (mPauseProgress < mFramePauses[mPauseIdx])
		{
			++mPauseProgress;
			return false;
		}
		else
		{
			mPauseProgress = 0;
			++mPauseIdx;
		}
	}
	return true;
}

