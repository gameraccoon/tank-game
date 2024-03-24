#include "Base/precomp.h"

#ifndef DISABLE_SDL

#include "SDL_surface.h"
#include <glew/glew.h>
#include <glm/gtc/matrix_transform.hpp>

#include "Base/Debug/ConcurrentAccessDetector.h"

#include "HAL/Graphics/Font.h"
#include "HAL/Graphics/Renderer.h"
#include "HAL/Graphics/SdlSurface.h"

static constexpr double MATH_PI = 3.14159265358979323846;

#ifdef CONCURRENT_ACCESS_DETECTION
namespace HAL
{
	extern ConcurrentAccessDetector gSDLAccessDetector;
}
#endif

namespace HAL::Graphics
{
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
	}
} // namespace HAL::Graphics

#endif // !DISABLE_SDL
