#pragma once

#ifndef DISABLE_SDL

#include <memory>
#include <functional>

#include "HAL/Graphics/RenderDeviceType.h"

namespace Diligent
{
	struct ISwapChain;
	struct IRenderDevice;
	struct IPipelineState;
	struct IDeviceContext;
	struct IEngineFactory;
}

namespace HAL
{
	class Window;

	namespace Graphics
	{
		class DiligentEngine
		{
		public:
			using CreateResourcesCallback = std::function<void(Diligent::ISwapChain&, Diligent::IRenderDevice&, Diligent::IEngineFactory&, Diligent::IPipelineState**)>;
			using RenderCallback = std::function<void(Diligent::ISwapChain&, Diligent::IDeviceContext&, Diligent::IPipelineState&)>;

		public:
			DiligentEngine(Window& window, RendererDeviceType renderDeviceType);
			~DiligentEngine();

			void setCreateResourcesCallback(CreateResourcesCallback&& callback);
			void setRenderCallback(RenderCallback&& callback);

			void createResources();
			void render();
			void present();
			void windowResize(int width, int height);

		private:
			class Impl;
			std::unique_ptr<Impl> mPimpl;
		};
	} // namespace Graphics
} // namespace HAL

#endif // !DISABLE_SDL
