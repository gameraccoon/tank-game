#pragma once

#ifndef DISABLE_SDL

namespace HAL
{
	namespace Graphics
	{
		enum class RendererDeviceType
		{
			Undefined,
			OpenGL,
			Vulkan
		};
	}
}

#endif // !DISABLE_SDL
