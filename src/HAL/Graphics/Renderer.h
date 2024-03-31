#pragma once

#ifndef DISABLE_SDL

#include <array>
#include <vector>
#include <memory>

#include <glm/fwd.hpp>

#include "GameData/Geometry/Vector2D.h"

#include "HAL/Base/Types.h"
#include "HAL/EngineFwd.h"
#include "HAL/Graphics/Diligent/DiligentEngine.h"

namespace HAL::Graphics
{
	class Surface;
	class DiligentEngine;

	namespace Render
	{
		void BindSurface(const Surface& surface);
		void DrawQuad(const glm::mat4& transform, Vector2D size, QuadUV uv, float alpha = 1.0f);
		void DrawQuad(Vector2D pos, Vector2D size);
		void DrawQuad(Vector2D pos, Vector2D size, Vector2D anchor, float rotation, QuadUV uv, float alpha = 1.0f);
		void DrawFan(const std::vector<DrawPoint>& points, const glm::mat4& transform, float alpha);
		void DrawStrip(const std::vector<DrawPoint>& points, const glm::mat4& transform, float alpha);
		void DrawTiledQuad(Vector2D start, Vector2D size, const QuadUV& uv);
	}

	class Renderer
	{
	public:
		Renderer(Window& window, RendererDeviceType renderDeviceType);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer(Renderer&&) = delete;
		Renderer& operator=(Renderer&&) = delete;

		DiligentEngine& getDiligentEngine();

	private:
		struct Impl;
		std::unique_ptr<Impl> mPimpl;
	};
} // namespace HAL::Graphics

#endif // !DISABLE_SDL
