#pragma once

#ifndef DEDICATED_SERVER

#include <cstring> // should be before SDL.h to suppress undifined memcpy error
#include <SDL.h>

namespace HAL
{
	namespace Internal
	{
		class Window;

		class GlContext
		{
		public:
			explicit GlContext(Window& window);

			GlContext(const GlContext&) = delete;
			GlContext operator=(const GlContext&) = delete;
			GlContext(GlContext&&) = delete;
			GlContext operator=(GlContext&&) = delete;

			~GlContext();

			SDL_GLContext getRawGLContext();

		private:
			SDL_GLContext mContext;
		};
	}
}

#endif // !DEDICATED_SERVER
