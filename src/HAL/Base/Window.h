#pragma once

#ifndef DISABLE_SDL

#include "HAL/Graphics/RenderDeviceType.h"

struct SDL_Window;

namespace HAL
{
	class Window
	{
	public:
		Window(Graphics::RendererDeviceType renderDeviceType, int width, int height);
		Window(const Window&) = delete;
		Window& operator=(const Window&) = delete;
		Window(Window&&) noexcept = delete;
		Window& operator=(Window&&) noexcept = delete;
		~Window();

		bool isValid() const;
		SDL_Window* getRawWindow();

		void show();
		void hide();

	private:
		SDL_Window* mSdlWindow = nullptr;
	};
}

#endif // !DISABLE_SDL
