#pragma once

#include "Base/Types/BasicTypes.h"

class FrameTimeCorrector
{
public:
	void updateIndexShift(s32 indexShift);
	void advanceOneUpdate();
	std::chrono::duration<s64, std::milli> getFrameLengthCorrection() const;

private:
	s32 mLastRequestedCorrection = 0;
};
