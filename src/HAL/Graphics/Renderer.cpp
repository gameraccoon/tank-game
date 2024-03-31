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
#pragma clang diagnostic ignored "-Wgnu-anonymous-struct"
#pragma clang diagnostic ignored "-Wnested-anon-types"
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif // __clang__
#include "Common/interface/RefCntAutoPtr.hpp"
#include "Common/interface/BasicMath.hpp"
#include "Graphics/GraphicsAccessories/interface/ColorConversion.h"
#include "Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "Graphics/GraphicsEngine/interface/SwapChain.h"
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
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
	struct Renderer::Impl
	{
		DiligentEngine mDiligentEngine;
		Diligent::RefCntAutoPtr<Diligent::IBuffer> mVertexBuffer;
		Diligent::RefCntAutoPtr<Diligent::IBuffer> mIndexBuffer;
		bool mConvertPsOutputToGamma = false;
		Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> mSrb;
		Diligent::RefCntAutoPtr<Diligent::ITextureView> mTextureSrv;

		Impl(Window& window, RendererDeviceType renderDeviceType)
			: mDiligentEngine(window, renderDeviceType)
		{
			mDiligentEngine.setCreateResourcesCallback([this](auto&&... var){ createResources(std::forward<decltype(var)>(var)...); });
			mDiligentEngine.setRenderCallback([this](auto&&... var){ render(std::forward<decltype(var)>(var)...); });
		}

		void createPipelineState(Diligent::ISwapChain& swapChain, Diligent::IRenderDevice& device, Diligent::IEngineFactory& engineFactory, Diligent::IPipelineState** pso)
		{
			const Diligent::TEXTURE_FORMAT colorBufferFormat = swapChain.GetDesc().ColorBufferFormat;
			const Diligent::TEXTURE_FORMAT depthBufferFormat = swapChain.GetDesc().DepthBufferFormat;

			Diligent::GraphicsPipelineStateCreateInfo psoCreateInfo;

			Diligent::PipelineStateDesc& psoDesc = psoCreateInfo.PSODesc;
			psoDesc.Name = "Simple triangle PSO";
			psoDesc.PipelineType = Diligent::PIPELINE_TYPE_GRAPHICS;
			psoCreateInfo.GraphicsPipeline.NumRenderTargets = 1;
			psoCreateInfo.GraphicsPipeline.RTVFormats[0] = colorBufferFormat;
			psoCreateInfo.GraphicsPipeline.DSVFormat = depthBufferFormat;
			psoCreateInfo.GraphicsPipeline.PrimitiveTopology = Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			// For simple 2D we don't need back-face culling and depth testing
			psoCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode = Diligent::CULL_MODE_NONE;
			psoCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable = Diligent::False;

			Diligent::ShaderCreateInfo shaderCI;
			shaderCI.SourceLanguage = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;
			// OpenGL backend requires emulated combined HLSL texture samplers (g_Texture + g_Texture_sampler combination)
			shaderCI.Desc.UseCombinedTextureSamplers = true;

			// Presentation engine always expects input in gamma space. Normally, pixel shader output is
			// converted from linear to gamma space by the GPU. However, some platforms (e.g. Android in GLES mode,
			// or Emscripten in WebGL mode) do not support gamma-correction. In this case the application
			// has to do the conversion manually.
			// If the swap chain color buffer format is a non-sRGB UNORM format,
			// we need to manually convert pixel shader output to gamma space.
			Diligent::ShaderMacro psMacros[] = { { "CONVERT_PS_OUTPUT_TO_GAMMA", mConvertPsOutputToGamma ? "1" : "0" } };
			shaderCI.Macros = { psMacros, _countof(psMacros) };

			// Create a shader source stream factory to load shaders from files.
			Diligent::RefCntAutoPtr<Diligent::IShaderSourceInputStreamFactory> shaderSourceFactory;
			engineFactory.CreateDefaultShaderSourceStreamFactory(nullptr, &shaderSourceFactory);
			shaderCI.pShaderSourceStreamFactory = shaderSourceFactory;

			Diligent::RefCntAutoPtr<Diligent::IShader> vertexShader;
			{
				shaderCI.Desc.ShaderType = Diligent::SHADER_TYPE_VERTEX;
				shaderCI.EntryPoint = "main";
				shaderCI.Desc.Name = "Triangle vertex shader";
				shaderCI.FilePath = "resources/shaders/simple_mesh.vsh";
				device.CreateShader(shaderCI, &vertexShader);
			}

			Diligent::RefCntAutoPtr<Diligent::IShader> pixelShader;
			{
				shaderCI.Desc.ShaderType = Diligent::SHADER_TYPE_PIXEL;
				shaderCI.EntryPoint = "main";
				shaderCI.Desc.Name = "Triangle pixel shader";
				shaderCI.FilePath = "resources/shaders/simple_mesh.psh";
				device.CreateShader(shaderCI, &pixelShader);
			}

			psoCreateInfo.pVS = vertexShader;
			psoCreateInfo.pPS = pixelShader;

			// Define vertex shader input layout
			Diligent::LayoutElement layoutElems[] = {
				// Attribute 0 - vertex position
				Diligent::LayoutElement{ 0, 0, 2, Diligent::VT_FLOAT32, Diligent::False },
				// Attribute 1 - texture coordinates
				Diligent::LayoutElement{ 1, 0, 2, Diligent::VT_FLOAT32, Diligent::False },
			};

			psoCreateInfo.GraphicsPipeline.InputLayout.LayoutElements = layoutElems;
			psoCreateInfo.GraphicsPipeline.InputLayout.NumElements = _countof(layoutElems);

			psoCreateInfo.PSODesc.ResourceLayout.DefaultVariableType = Diligent::SHADER_RESOURCE_VARIABLE_TYPE_STATIC;

			// Shader variables should typically be mutable, which means they are expected
			// to change on a per-instance basis
			Diligent::ShaderResourceVariableDesc Vars[] = {
				{ Diligent::SHADER_TYPE_PIXEL, "g_Texture", Diligent::SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE }
			};
			psoCreateInfo.PSODesc.ResourceLayout.Variables = Vars;
			psoCreateInfo.PSODesc.ResourceLayout.NumVariables = _countof(Vars);

			// Define immutable sampler for g_Texture. Immutable samplers should be used whenever possible
			Diligent::SamplerDesc samLinearClampDesc{
				Diligent::FILTER_TYPE_LINEAR, Diligent::FILTER_TYPE_LINEAR, Diligent::FILTER_TYPE_LINEAR,
				Diligent::TEXTURE_ADDRESS_CLAMP, Diligent::TEXTURE_ADDRESS_CLAMP, Diligent::TEXTURE_ADDRESS_CLAMP
			};
			Diligent::ImmutableSamplerDesc imtblSamplers[] = {
				{ Diligent::SHADER_TYPE_PIXEL, "g_Texture", samLinearClampDesc }
			};
			psoCreateInfo.PSODesc.ResourceLayout.ImmutableSamplers = imtblSamplers;
			psoCreateInfo.PSODesc.ResourceLayout.NumImmutableSamplers = _countof(imtblSamplers);

			device.CreateGraphicsPipelineState(psoCreateInfo, pso);

			// Since we are using mutable variable, we must create a shader resource binding object
			// http://diligentgraphics.com/2016/03/23/resource-binding-model-in-diligent-engine-2-0/
			(*pso)->CreateShaderResourceBinding(&mSrb, true);
		}

		void loadTextures(Diligent::IRenderDevice& device)
		{
			{
				Diligent::TextureDesc texDesc;
				texDesc.Name = "Test texture";
				texDesc.Width = 100;
				texDesc.Height = 200;
				texDesc.Type = Diligent::RESOURCE_DIM_TEX_2D;
				texDesc.Format = Diligent::TEX_FORMAT_R8_UNORM;
				texDesc.BindFlags = Diligent::BIND_SHADER_RESOURCE;

				Diligent::RefCntAutoPtr<Diligent::ITexture> texture;
				device.CreateTexture(texDesc, nullptr, &texture);
				Assert(texture != nullptr, "Failed to create texture");
				mTextureSrv = texture->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE);
			}

			mSrb->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_Texture")->Set(mTextureSrv);
		}

		void createVertexBuffer(Diligent::IRenderDevice& device)
		{
			// Layout of this structure matches the one we defined in the pipeline state
			struct Vertex
			{
				Diligent::float2 pos;
				Diligent::float2 uv;
			};

			constexpr Vertex verts[] = {
				{ Diligent::float2{ -0.5f, -0.5f }, Diligent::float2{ 0.0f, 0.0f } },
				{ Diligent::float2{ 0.0f, 0.5f }, Diligent::float2{ 0.5f, 1.0f } },
				{ Diligent::float2{ 0.5f, -0.5f }, Diligent::float2{ 1.0f, 0.0f } },
			};

			Diligent::BufferDesc vertBuffDesc;
			vertBuffDesc.Name = "Simple vertex buffer";
			vertBuffDesc.Usage = Diligent::USAGE_IMMUTABLE;
			vertBuffDesc.BindFlags = Diligent::BIND_VERTEX_BUFFER;
			vertBuffDesc.Size = sizeof(verts);
			Diligent::BufferData vbData;
			vbData.pData = verts;
			vbData.DataSize = sizeof(verts);
			device.CreateBuffer(vertBuffDesc, &vbData, &mVertexBuffer);
		}

		void createIndexBuffer(Diligent::IRenderDevice& device)
		{
			// clang-format off
			constexpr Uint32 indices[] =
			{
				0, 1, 2,
			};
			// clang-format on

			Diligent::BufferDesc indBuffDesc;
			indBuffDesc.Name = "Simple index buffer";
			indBuffDesc.Usage = Diligent::USAGE_IMMUTABLE;
			indBuffDesc.BindFlags = Diligent::BIND_INDEX_BUFFER;
			indBuffDesc.Size = sizeof(indices);
			Diligent::BufferData ibData;
			ibData.pData = indices;
			ibData.DataSize = sizeof(indices);
			device.CreateBuffer(indBuffDesc, &ibData, &mIndexBuffer);
		}

		void createResources(Diligent::ISwapChain& swapChain, Diligent::IRenderDevice& device, Diligent::IEngineFactory& engineFactory, Diligent::IPipelineState** pso)
		{
			Diligent::TEXTURE_FORMAT colorBufferFormat = swapChain.GetDesc().ColorBufferFormat;
			mConvertPsOutputToGamma = (colorBufferFormat == Diligent::TEX_FORMAT_RGBA8_UNORM || colorBufferFormat == Diligent::TEX_FORMAT_BGRA8_UNORM);

			createPipelineState(swapChain, device, engineFactory, pso);
			loadTextures(device);
			createVertexBuffer(device);
			createIndexBuffer(device);
		}

		void render(Diligent::ISwapChain& swapChain, Diligent::IDeviceContext& context, Diligent::IPipelineState& pso) const
		{
			auto* pRtv = swapChain.GetCurrentBackBufferRTV();
			auto* pDsv = swapChain.GetDepthBufferDSV();
			// Clear the back buffer
			Diligent::float4 clearColor = { 0.350f, 0.350f, 0.350f, 1.0f };
			if (mConvertPsOutputToGamma)
			{
				// If manual gamma correction is required, we need to clear the render target with sRGB color
				clearColor = Diligent::LinearToSRGB(clearColor);
			}
			context.ClearRenderTarget(pRtv, clearColor.Data(), Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
			context.ClearDepthStencil(pDsv, Diligent::CLEAR_DEPTH_FLAG, 1.f, 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

			// Bind vertex and index buffers
			const Uint64 offset = 0;
			Diligent::IBuffer* pBuffs[] = { mVertexBuffer };
			context.SetVertexBuffers(0, 1, pBuffs, &offset, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION, Diligent::SET_VERTEX_BUFFERS_FLAG_RESET);
			context.SetIndexBuffer(mIndexBuffer, 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

			// Set the pipeline state
			context.SetPipelineState(&pso);
			// Commit shader resources. RESOURCE_STATE_TRANSITION_MODE_TRANSITION mode
			// makes sure that resources are transitioned to required states.
			context.CommitShaderResources(mSrb, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

			Diligent::DrawIndexedAttribs drawAttrs;    // This is an indexed draw call
			drawAttrs.IndexType = Diligent::VT_UINT32; // Index type
			drawAttrs.NumIndices = 3;
			// Verify the state of vertex and index buffers
			drawAttrs.Flags = Diligent::DRAW_FLAG_VERIFY_ALL;
			context.DrawIndexed(drawAttrs);
		}
	};

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
		: mPimpl(std::make_unique<Impl>(window, renderDeviceType))
	{
	}

	Renderer::~Renderer() = default;

	DiligentEngine& Renderer::getDiligentEngine()
	{
		return mPimpl->mDiligentEngine;
	}
} // namespace HAL::Graphics

#endif // !DISABLE_SDL
