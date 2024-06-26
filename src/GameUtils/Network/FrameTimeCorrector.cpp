#include "GameUtils/Network/FrameTimeCorrector.h"

#include "EngineCommon/TimeConstants.h"

void FrameTimeCorrector::updateIndexShift(s32 indexShift)
{
	if (mCorrectionLeft.count() == 0) {
		mCorrectionLeft = indexShift * TimeConstants::ONE_FIXED_UPDATE_DURATION;
		return;
	}

	const auto suggestedCorrection = indexShift * TimeConstants::ONE_FIXED_UPDATE_DURATION;

	// if the suggested correction is bigger than the current, it just means we're getting old data
	// however if the value is less or has different sign, we should update the correction

	const bool previousCorrectionSign = mCorrectionLeft.count() > 0;
	const bool suggestedCorrectionSign = suggestedCorrection.count() > 0;
	if (previousCorrectionSign != suggestedCorrectionSign)
	{
		mCorrectionLeft = suggestedCorrection;
	}

	const auto previousCorrectionAbs = std::chrono::abs(mCorrectionLeft);
	const auto suggestedCorrectionAbs = std::chrono::abs(suggestedCorrection);
	if (suggestedCorrectionAbs < previousCorrectionAbs)
	{
		mCorrectionLeft = suggestedCorrection;
	}
}

void FrameTimeCorrector::advanceOneUpdate()
{
	if (mCorrectionLeft.count() == 0)
	{
		return;
	}

	mCorrectionLeft -= getFrameLengthCorrection();
}

std::chrono::duration<s64, std::micro> FrameTimeCorrector::getFrameLengthCorrection() const
{
	static const std::chrono::microseconds ONE_UPDATE_TIME = TimeConstants::ONE_FIXED_UPDATE_DURATION;
	static const std::chrono::microseconds MIN_CORRECTION = ONE_UPDATE_TIME / 50;
	// don't drop below 15 fps
	static const std::chrono::microseconds MAX_CORRECTION_POSITIVE = ONE_UPDATE_TIME * 3;
	// don't go over 240 fps
	static const std::chrono::microseconds MAX_CORRECTION_NEGATIVE = ONE_UPDATE_TIME * 3 / 4;
	static const int CORRECTION_FRACTIONS = 6;

	if (mCorrectionLeft.count() == 0)
	{
		return mCorrectionLeft;
	}

	const auto correctionAbs = std::chrono::abs(mCorrectionLeft);

	// if this is the last step, just apply it
	if (correctionAbs < MIN_CORRECTION)
	{
		return mCorrectionLeft;
	}

	// apply only a fraction of the correction
	// the more left of it, the faster it will be applied

	const int correctionSign = mCorrectionLeft.count() > 0 ? 1 : -1;
	const auto correctionFraction = std::max(correctionAbs / CORRECTION_FRACTIONS, MIN_CORRECTION);

	// cap the correction per frame, so it doesn't go overboard
	if (correctionSign > 0)
	{
		return std::min(correctionFraction, MAX_CORRECTION_POSITIVE);
	}
	else
	{
		return -std::min(correctionFraction, MAX_CORRECTION_NEGATIVE);
	}
}
