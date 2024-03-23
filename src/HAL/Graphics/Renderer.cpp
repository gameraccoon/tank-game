#include "Base/precomp.h"

#ifndef DISABLE_SDL

#include <SDL.h>
#include <SDL_syswm.h>
#include <SDL_surface.h>

#include <glew/glew.h>
#include <glm/gtc/matrix_transform.hpp>

#include "Base/Debug/ConcurrentAccessDetector.h"

#include "HAL/Graphics/Font.h"
#include "HAL/Graphics/Renderer.h"
#include "HAL/Graphics/SdlSurface.h"

#include "HAL/Base/SdlInstance.h"

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

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#endif // __clang__
#include "Common/interface/RefCntAutoPtr.hpp"
#include "Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "Graphics/GraphicsEngine/interface/SwapChain.h"
#if defined(__clang__)
#pragma clang diagnostic pop
#endif // __clang__

static constexpr double MATH_PI = 3.14159265358979323846;

#ifdef CONCURRENT_ACCESS_DETECTION
namespace HAL
{
	extern ConcurrentAccessDetector gSDLAccessDetector;
}
#endif

namespace HAL::Graphics
{
	static void createResources(Diligent::ISwapChain& swapChain, Diligent::IRenderDevice& device, Diligent::IEngineFactory& engineFactory, Diligent::IPipelineState** pso)
	{
		// Pipeline state object encompasses configuration of all GPU stages

		Diligent::GraphicsPipelineStateCreateInfo psoCreateInfo;
		Diligent::PipelineStateDesc& psoDesc = psoCreateInfo.PSODesc;

		// Pipeline state name is used by the engine to report issues
		// It is always a good idea to give objects descriptive names
		psoDesc.Name = "Simple triangle PSO";

		// This is a graphics pipeline
		psoDesc.PipelineType = Diligent::PIPELINE_TYPE_GRAPHICS;

		// This tutorial will render to a single render target
		psoCreateInfo.GraphicsPipeline.NumRenderTargets = 1;
		// Set render target format which is the format of the swap chain's color buffer
		psoCreateInfo.GraphicsPipeline.RTVFormats[0] = swapChain.GetDesc().ColorBufferFormat;
		// This tutorial will not use depth buffer
		psoCreateInfo.GraphicsPipeline.DSVFormat = swapChain.GetDesc().DepthBufferFormat;
		// Primitive topology defines what kind of primitives will be rendered by this pipeline state
		psoCreateInfo.GraphicsPipeline.PrimitiveTopology = Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		// No back face culling for this tutorial
		psoCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode = Diligent::CULL_MODE_NONE;
		// Disable depth testing
		psoCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable = Diligent::False;

		Diligent::ShaderCreateInfo shaderCI;
		// Tell the system that the shader source code is in HLSL.
		// For OpenGL, the engine will convert this into GLSL behind the scene
		shaderCI.SourceLanguage = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;
		// OpenGL backend requires emulated combined HLSL texture samplers (g_Texture + g_Texture_sampler combination)
		shaderCI.Desc.UseCombinedTextureSamplers = true;

		Diligent::TEXTURE_FORMAT colorBufferFormat = swapChain.GetDesc().ColorBufferFormat;
		// Presentation engine always expects input in gamma space. Normally, pixel shader output is
		// converted from linear to gamma space by the GPU. However, some platforms (e.g. Android in GLES mode,
		// or Emscripten in WebGL mode) do not support gamma-correction. In this case the application
		// has to do the conversion manually.
		// If the swap chain color buffer format is a non-sRGB UNORM format,
		// we need to manually convert pixel shader output to gamma space.
		const bool convertPsOutputToGamma = (colorBufferFormat == Diligent::TEX_FORMAT_RGBA8_UNORM || colorBufferFormat == Diligent::TEX_FORMAT_BGRA8_UNORM);
		Diligent::ShaderMacro psMacros[] = { { "CONVERT_PS_OUTPUT_TO_GAMMA", convertPsOutputToGamma ? "1" : "0" } };
		shaderCI.Macros = { psMacros, _countof(psMacros) };

		// Create a shader source stream factory to load shaders from files.
		Diligent::RefCntAutoPtr<Diligent::IShaderSourceInputStreamFactory> shaderSourceFactory;
		engineFactory.CreateDefaultShaderSourceStreamFactory(nullptr, &shaderSourceFactory);
		shaderCI.pShaderSourceStreamFactory = shaderSourceFactory;

		// Create vertex shader
		Diligent::RefCntAutoPtr<Diligent::IShader> vectorShader;
		{
			shaderCI.Desc.ShaderType = Diligent::SHADER_TYPE_VERTEX;
			shaderCI.EntryPoint = "main";
			shaderCI.Desc.Name = "Triangle vertex shader";
			shaderCI.FilePath = "resources/shaders/triangle.vsh";
			device.CreateShader(shaderCI, &vectorShader);
		}

		// Create pixel shader
		Diligent::RefCntAutoPtr<Diligent::IShader> pixelShader;
		{
			shaderCI.Desc.ShaderType = Diligent::SHADER_TYPE_PIXEL;
			shaderCI.EntryPoint = "main";
			shaderCI.Desc.Name = "Triangle pixel shader";
			shaderCI.FilePath = "resources/shaders/triangle.psh";
			device.CreateShader(shaderCI, &pixelShader);
		}

		// Finally, create the pipeline state
		psoCreateInfo.pVS = vectorShader;
		psoCreateInfo.pPS = pixelShader;
		device.CreateGraphicsPipelineState(psoCreateInfo, pso);
	}

	static void render(Diligent::ISwapChain& swapChain, Diligent::IDeviceContext& context, Diligent::IPipelineState& pso)
	{
		// Set render targets before issuing any draw command.
		// Note that Present() unbinds the back buffer if it is set as render target.
		auto* pRtv = swapChain.GetCurrentBackBufferRTV();
		auto* pDsv = swapChain.GetDepthBufferDSV();
		context.SetRenderTargets(1, &pRtv, pDsv, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

		// Clear the back buffer
		const float clearColor[] = { 0.350f, 0.350f, 0.350f, 1.0f };
		// Let the engine perform required state transitions
		context.ClearRenderTarget(pRtv, clearColor, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
		context.ClearDepthStencil(pDsv, Diligent::CLEAR_DEPTH_FLAG, 1.f, 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

		// Set pipeline state in the immediate context
		context.SetPipelineState(&pso);

		// Typically we should now call CommitShaderResources(), however shaders in this example don't
		// use any resources.

		Diligent::DrawAttribs drawAttrs;
		drawAttrs.NumVertices = 3; // We will render 3 vertices
		context.Draw(drawAttrs);
	}

	void Render::BindSurface(const Surface& surface)
	{
		surface.bind();
	}

	void Render::DrawQuad(const glm::mat4& transform, Vector2D size, Graphics::QuadUV uv, float alpha)
	{
		DETECT_CONCURRENT_ACCESS(HAL::gSDLAccessDetector);
		glLoadMatrixf(reinterpret_cast<const float*>(&transform));

		glBegin(GL_QUADS);
		glColor4f(1.0f, 1.0f, 1.0f, alpha);
		glTexCoord2f(uv.u1, uv.v2); glVertex2f(0.0f, size.y);
		glTexCoord2f(uv.u2, uv.v2); glVertex2f(size.x, size.y);
		glTexCoord2f(uv.u2, uv.v1); glVertex2f(size.x, 0.0f);
		glTexCoord2f(uv.u1, uv.v1); glVertex2f(0.0f, 0.0f);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		glEnd();
	}

	void Render::DrawQuad(Vector2D pos, Vector2D size)
	{
		DETECT_CONCURRENT_ACCESS(HAL::gSDLAccessDetector);
		glm::mat4 transform{ 1.0f };
		transform = glm::translate(transform, glm::vec3(pos.x, pos.y, 0.0f));
		glLoadMatrixf(reinterpret_cast<const float*>(&transform));

		glBegin(GL_QUADS);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		glTexCoord2f(0.0f, 1.0f); glVertex2f(0.0f, size.y);
		glTexCoord2f(1.0f, 1.0f); glVertex2f(size.x, size.y);
		glTexCoord2f(1.0f, 0.0f); glVertex2f(size.x, 0.0f);
		glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, 0.0f);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		glEnd();
	}

	void Render::DrawQuad(Vector2D pos, Vector2D size, Vector2D anchor, float rotation, Graphics::QuadUV uv, float alpha)
	{
		glm::mat4 transform{ 1.0f };
		transform = glm::translate(transform, glm::vec3(pos.x, pos.y, 0.0f));
		transform = glm::rotate(transform, rotation, glm::vec3(0.0f, 0.0f, 1.0f));
		transform = glm::translate(transform, glm::vec3(-size.x * anchor.x, -size.y * anchor.y, 0.0f));
		DrawQuad(transform, size, uv, alpha);
	}

	void Render::DrawFan(const std::vector<DrawPoint>& points, const glm::mat4& transform, float alpha)
	{
		DETECT_CONCURRENT_ACCESS(HAL::gSDLAccessDetector);
		glLoadMatrixf(reinterpret_cast<const float*>(&transform));

		glBegin(GL_TRIANGLE_FAN);
		glColor4f(1.0f, 1.0f, 1.0f, alpha);
		for (const DrawPoint& point : points)
		{
			glTexCoord2f(point.texturePoint.x, point.texturePoint.y);
			glVertex2f(point.spacePoint.x, point.spacePoint.y);
		}
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		glEnd();
	}

	void Render::DrawStrip(const std::vector<DrawPoint>& points, const glm::mat4& transform, float alpha)
	{
		DETECT_CONCURRENT_ACCESS(HAL::gSDLAccessDetector);
		glLoadMatrixf(reinterpret_cast<const float*>(&transform));

		glBegin(GL_TRIANGLE_STRIP);
		glColor4f(1.0f, 1.0f, 1.0f, alpha);
		for (const DrawPoint& point : points)
		{
			glTexCoord2f(point.texturePoint.x, point.texturePoint.y);
			glVertex2f(point.spacePoint.x, point.spacePoint.y);
		}
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		glEnd();
	}

	void Render::DrawTiledQuad(Vector2D start, Vector2D size, const QuadUV& uv)
	{
		DETECT_CONCURRENT_ACCESS(HAL::gSDLAccessDetector);
		glm::mat4 transform{ 1.0f };
		glLoadMatrixf(reinterpret_cast<const float*>(&transform));

		glBegin(GL_QUADS);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		glTexCoord2f(uv.u1, uv.v2); glVertex2f(start.x, start.y + size.y);
		glTexCoord2f(uv.u2, uv.v2); glVertex2f(start.x + size.x, start.y + size.y);
		glTexCoord2f(uv.u2, uv.v1); glVertex2f(start.x + size.x, start.y);
		glTexCoord2f(uv.u1, uv.v1); glVertex2f(start.x, start.y);
		glEnd();
	}

	Renderer::Renderer(Window& window, RendererDeviceType renderDeviceType)
		: mDiligentEngine(window, renderDeviceType)
	{
		mDiligentEngine.setCreateResourcesCallback(createResources);
		mDiligentEngine.setRenderCallback(render);
	}
} // namespace HAL::Graphics

#endif // !DISABLE_SDL
