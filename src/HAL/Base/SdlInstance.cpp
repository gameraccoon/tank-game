#include "Base/precomp.h"

#ifndef DISABLE_SDL

#include <stdexcept>

#include "SDL/include/SDL.h"
#include "SdlInstance.h"

namespace HAL
{
	SdlInstance::SdlInstance(unsigned int flags)
		: mFlags(flags)
	{
		if (SDL_InitSubSystem(mFlags) != 0)
		{
			ReportError("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
		}
	}

	SdlInstance::~SdlInstance()
	{
		SDL_QuitSubSystem(mFlags);
	}
}

#endif // !DISABLE_SDL
