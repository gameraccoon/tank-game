#include "Base/precomp.h"

#include "HAL/Graphics/Diligent/DiligentEngine.h"

#ifndef DISABLE_SDL

#include <SDL.h>

#include <SDL_syswm.h>

// Undef symbols defined by XLib
#ifdef Bool
#undef Bool
#endif
#ifdef True
#undef True
#endif
#ifdef False
#undef False
#endif
#ifdef None
#undef None
#endif
#ifdef Status
#undef Status
#endif
#ifdef Success
#undef Success
#endif

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif // __clang__

#if GL_SUPPORTED
#include "Graphics/GraphicsEngineOpenGL/interface/EngineFactoryOpenGL.h"
#endif // GL_SUPPORTED

#if VULKAN_SUPPORTED
#include "Graphics/GraphicsEngineVulkan/interface/EngineFactoryVk.h"
#endif // VULKAN_SUPPORTED

#include "Common/interface/RefCntAutoPtr.hpp"
#include "Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "Graphics/GraphicsEngine/interface/SwapChain.h"

#if defined(__clang__)
#pragma clang diagnostic pop
#endif // __clang__

#include "HAL/Base/Window.h"

namespace HAL
{
	namespace Graphics
	{
		class DiligentEngine::Impl
		{
		public:
			Impl() = default;
			~Impl()
			{
				mImmediateContext->Flush();
			}

			void setCreateResourcesCallback(std::function<void(Diligent::ISwapChain&, Diligent::IRenderDevice&, Diligent::IEngineFactory&, Diligent::IPipelineState**)>&& createResourcesCallback)
			{
				mCreateResourcesCallback = std::move(createResourcesCallback);
			}

			void setRenderCallback(std::function<void(Diligent::ISwapChain&, Diligent::IDeviceContext&, Diligent::IPipelineState&)>&& renderCallback)
			{
				mRenderCallback = std::move(renderCallback);
			}

			void createResources()
			{
				if (mCreateResourcesCallback)
				{
					mCreateResourcesCallback(*mSwapChain, *mDevice, *mEngineFactory, &mPso);
				}
			}

			void render()
			{
				if (mRenderCallback)
				{
					mRenderCallback(*mSwapChain, *mImmediateContext, *mPso);
				}
			}

#if GL_SUPPORTED
			static std::unique_ptr<DiligentEngine::Impl> createForGL(SDL_Window* window)
			{
				if (window == nullptr)
				{
					return nullptr;
				}

				SDL_GL_CreateContext(window);
				SDL_GL_MakeCurrent(window, SDL_GL_GetCurrentContext());

				SDL_SysWMinfo wmInfo;
				SDL_VERSION(&wmInfo.version); // Initialize wmInfo to the SDL version we compiled against
				if (SDL_GetWindowWMInfo(window, &wmInfo) == SDL_FALSE)
				{
					printf("Failed to get WM info! SDL Error: %s\n", SDL_GetError());
				}

				Diligent::SwapChainDesc swapChainDesc;
				Diligent::IEngineFactoryOpenGL* engineFactoryGl = Diligent::GetEngineFactoryOpenGL();

				Diligent::EngineGLCreateInfo engineCreateInfo;
#ifdef PLATFORM_LINUX
				engineCreateInfo.Window.pDisplay = wmInfo.info.x11.display;
				engineCreateInfo.Window.WindowId = wmInfo.info.x11.window;
#endif // PLATFORM_LINUX
#ifdef PLATFORM_WIN32
				engineCreateInfo.Window.hWnd = wmInfo.info.win.window;
#endif // PLATFORM_WIN32

				std::unique_ptr<DiligentEngine::Impl> diligentEngine = std::make_unique<DiligentEngine::Impl>();
				engineFactoryGl->CreateDeviceAndSwapChainGL(engineCreateInfo, &diligentEngine->mDevice, &diligentEngine->mImmediateContext, swapChainDesc, &diligentEngine->mSwapChain);
				diligentEngine->mEngineFactory = engineFactoryGl;

				return diligentEngine;
			}
#endif // GL_SUPPORTED

#if VULKAN_SUPPORTED
			static std::unique_ptr<DiligentEngine::Impl> createForVulkan(SDL_Window* window)
			{
				if (window == nullptr)
				{
					return nullptr;
				}

				std::unique_ptr<DiligentEngine::Impl> diligentEngine = std::make_unique<DiligentEngine::Impl>();

				SDL_SysWMinfo wmInfo;
				SDL_VERSION(&wmInfo.version); // Initialize wmInfo to the SDL version we compiled against
				if (SDL_GetWindowWMInfo(window, &wmInfo) == SDL_FALSE)
				{
					printf("Failed to get WM info! SDL Error: %s\n", SDL_GetError());
				}

				Diligent::EngineVkCreateInfo EngineCreateInfo;
				Diligent::IEngineFactoryVk* engineFactoryVk = Diligent::GetEngineFactoryVk();
				engineFactoryVk->CreateDeviceAndContextsVk(EngineCreateInfo, &diligentEngine->mDevice, &diligentEngine->mImmediateContext);
				Diligent::SwapChainDesc swapChainDesc;
#ifdef PLATFORM_LINUX
				Diligent::LinuxNativeWindow nativeWindow;
				nativeWindow.WindowId = wmInfo.info.x11.window;
				nativeWindow.pDisplay = wmInfo.info.x11.display;
#endif // PLATFORM_LINUX
#if PLATFORM_WIN32
				Diligent::Win32NativeWindow nativeWindow;
				nativeWindow.hWnd = wmInfo.info.win.window;
#endif // PLATFORM_WIN32
				engineFactoryVk->CreateSwapChainVk(diligentEngine->mDevice, diligentEngine->mImmediateContext, swapChainDesc, nativeWindow, &diligentEngine->mSwapChain);
				diligentEngine->mEngineFactory = engineFactoryVk;

				return diligentEngine;
			}
#endif // VULKAN_SUPPORTED

			static std::unique_ptr<DiligentEngine::Impl> create(SDL_Window* window, HAL::Graphics::RendererDeviceType deviceType)
			{
#if VULKAN_SUPPORTED
				if (deviceType == HAL::Graphics::RendererDeviceType::Vulkan)
				{
					return createForVulkan(window);
				}
#endif // VULKAN_SUPPORTED

#if GL_SUPPORTED
				if (deviceType == HAL::Graphics::RendererDeviceType::OpenGL)
				{
					return createForGL(window);
				}
#endif // GL_SUPPORTED

				return std::unique_ptr<DiligentEngine::Impl>(nullptr);
			}

			void present()
			{
				mSwapChain->Present();
			}

			void windowResize(Uint32 width, Uint32 height)
			{
				if (mSwapChain)
				{
					mSwapChain->Resize(width, height);
				}
			}

		private:
			std::function<void(Diligent::ISwapChain&, Diligent::IRenderDevice&, Diligent::IEngineFactory&, Diligent::IPipelineState**)> mCreateResourcesCallback;
			std::function<void(Diligent::ISwapChain&, Diligent::IDeviceContext&, Diligent::IPipelineState&)> mRenderCallback;

			Diligent::RefCntAutoPtr<Diligent::IEngineFactory> mEngineFactory;
			Diligent::RefCntAutoPtr<Diligent::IRenderDevice> mDevice;
			Diligent::RefCntAutoPtr<Diligent::IDeviceContext> mImmediateContext;
			Diligent::RefCntAutoPtr<Diligent::ISwapChain> mSwapChain;
			Diligent::RefCntAutoPtr<Diligent::IPipelineState> mPso;
		};

		DiligentEngine::DiligentEngine(Window& window, RendererDeviceType renderDeviceType)
		{
			switch (renderDeviceType)
			{
#if GL_SUPPORTED
			case RendererDeviceType::OpenGL:
				mPimpl = Impl::createForGL(window.getRawWindow());
				break;
#endif // GL_SUPPORTED
#if VULKAN_SUPPORTED
			case RendererDeviceType::Vulkan:
				mPimpl = Impl::createForVulkan(window.getRawWindow());
				break;
#endif // VULKAN_SUPPORTED
			default:
				ReportFatalError("Unsupported renderer device type");
				break;
			}
		}

		DiligentEngine::~DiligentEngine() = default;

		void DiligentEngine::setCreateResourcesCallback(CreateResourcesCallback&& callback)
		{
			mPimpl->setCreateResourcesCallback(std::move(callback));
		}

		void DiligentEngine::setRenderCallback(RenderCallback&& callback)
		{
			mPimpl->setRenderCallback(std::move(callback));
		}

		void DiligentEngine::createResources()
		{
			mPimpl->createResources();
		}

		void DiligentEngine::render()
		{
			mPimpl->render();
		}

		void DiligentEngine::present()
		{
			mPimpl->present();
		}

		void DiligentEngine::windowResize(int width, int height)
		{
			mPimpl->windowResize(width, height);
		}
	} // namespace Graphics
} // namespace HAL

#endif // !DISABLE_SDL
