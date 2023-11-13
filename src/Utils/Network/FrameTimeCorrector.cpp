#include "Utils/Network/FrameTimeCorrector.h"

#include "Base/TimeConstants.h"

void FrameTimeCorrector::updateIndexShift(s32 indexShift)
{
	mLastRequestedCorrection = indexShift;
}

void FrameTimeCorrector::advanceOneUpdate()
{
}

std::chrono::duration<s64, std::milli> FrameTimeCorrector::getFrameLengthCorrection() const
{
	static const std::chrono::milliseconds ONE_UPDATE_TIME = TimeConstants::ONE_FIXED_UPDATE_DURATION;
	const s32 indexShift = mLastRequestedCorrection;

	if (indexShift == 0)
	{
		return std::chrono::duration<s64, std::milli>{ 0 };
	}

	return (indexShift > 0)
		? ONE_UPDATE_TIME / 16
		: -ONE_UPDATE_TIME / 16;
}
