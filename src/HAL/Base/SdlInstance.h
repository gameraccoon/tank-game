#pragma once

#ifndef DISABLE_SDL

namespace HAL
{
	class SdlInstance
	{
	public:
		explicit SdlInstance(unsigned int flags);

		SdlInstance(const SdlInstance&) = delete;
		SdlInstance operator=(const SdlInstance&) = delete;
		SdlInstance(SdlInstance&&) = delete;
		SdlInstance operator=(SdlInstance&&) = delete;

		~SdlInstance();

	private:
		int mFlags;
	};
}

#endif // !DISABLE_SDL
