#include "Base/precomp.h"

#ifndef DISABLE_SDL

#include "HAL/Base/Window.h"

#include <stdexcept>
#include <string>

#include "SDL/include/SDL.h"

namespace HAL
{
	Window::Window(Graphics::RendererDeviceType renderDeviceType, int width, int height)
	{
		if (renderDeviceType == Graphics::RendererDeviceType::OpenGL)
		{
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		}

		Uint32 renderFlags = 0;
		if (renderDeviceType == Graphics::RendererDeviceType::OpenGL)
		{
			renderFlags = SDL_WINDOW_OPENGL;
		}
		else if (renderDeviceType == Graphics::RendererDeviceType::Vulkan)
		{
			renderFlags = SDL_WINDOW_VULKAN;
		}

		mSdlWindow = SDL_CreateWindow("Game", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, renderFlags | SDL_WINDOW_HIDDEN);
		AssertFatal(mSdlWindow, "Window could not be created! SDL Error: %s\n", SDL_GetError());
	}

	Window::~Window()
	{
		if (mSdlWindow != nullptr)
		{
			SDL_DestroyWindow(mSdlWindow);
		}
	}

	void Window::show()
	{
		if (mSdlWindow != nullptr)
		{
			SDL_ShowWindow(mSdlWindow);
		}
	}

	void Window::hide()
	{
		if (mSdlWindow != nullptr)
		{
			SDL_HideWindow(mSdlWindow);
		}
	}

	bool Window::isValid() const
	{
		return mSdlWindow != nullptr;
	}

	SDL_Window* Window::getRawWindow()
	{
		return mSdlWindow;
	}
}

#endif // !DISABLE_SDL
