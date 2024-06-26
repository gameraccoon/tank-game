#pragma once

#include "EngineCommon/Types/BasicTypes.h"

class FrameTimeCorrector
{
public:
	void updateIndexShift(s32 indexShift);
	void advanceOneUpdate();
	std::chrono::duration<s64, std::micro> getFrameLengthCorrection() const;

private:
	std::chrono::duration<s64, std::micro> mCorrectionLeft{0};
	u32 mUpdatesToPeakCorrection = 0;
	u32 mUpdatesToResetCorrection = 0;
};
