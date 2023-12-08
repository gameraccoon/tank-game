#pragma once

#include <vector>

class UpdateLagger
{
public:
	explicit UpdateLagger(std::vector<int>&& framePauses);
	bool shouldUpdate();

private:
	const std::vector<int> mFramePauses;
	size_t mPauseIdx = 0;
	int mPauseProgress = 0;
};
