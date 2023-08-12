#include "Base/precomp.h"

#ifndef DEDICATED_SERVER

#include "Sdl.h"

#include <SDL.h>
#include <stdexcept>

namespace HAL
{
	namespace Internal
	{
		SDLInstance::SDLInstance(unsigned int flags)
		{
			if (SDL_Init(flags) != 0)
			{
				throw std::runtime_error("Failed to init SDL");
			}
		}

		SDLInstance::~SDLInstance()
		{
			SDL_Quit();
		}
	}
}

#endif // !DEDICATED_SERVER
