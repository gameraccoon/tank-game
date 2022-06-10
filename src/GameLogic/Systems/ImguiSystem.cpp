#include "Base/precomp.h"

#include "GameLogic/Systems/ImguiSystem.h"

#ifdef IMGUI_ENABLED

#include "imgui/imgui.h"
#include "imgui/examples/imgui_impl_sdl.h"
#include "imgui/examples/imgui_impl_opengl2.h"

#include <SDL.h>
#include <SDL_opengl.h>

#include "Base/Types/TemplateHelpers.h"

#include "GameData/Components/ImguiComponent.generated.h"
#include "GameData/Components/RenderAccessorComponent.generated.h"
#include "GameData/Components/TimeComponent.generated.h"
#include "GameData/GameData.h"
#include "GameData/World.h"

#include "HAL/Base/Engine.h"

#include "GameLogic/Render/RenderAccessor.h"


ImguiSystem::ImguiSystem(
		ImguiDebugData& debugData,
		HAL::Engine& engine) noexcept
	: mEngine(engine)
	, mDebugData(debugData)
{
}

void ImguiSystem::update()
{
	SCOPED_PROFILER("ImguiSystem::update");

	GameData& gameData = mDebugData.worldHolder.getGameData();

	// check if we need to render imgui
	ImguiComponent* imgui = gameData.getGameComponents().getOrAddComponent<ImguiComponent>();

	if (!imgui->getIsImguiVisible())
	{
		return;
	}

	auto [renderAccessorCmp] = gameData.getGameComponents().getComponents<RenderAccessorComponent>();
	if (renderAccessorCmp == nullptr || !renderAccessorCmp->getAccessor().has_value())
	{
		return;
	}

	{
		std::lock_guard l(mRenderDataMutex);

		const auto [time] = mDebugData.worldHolder.getWorld().getWorldComponents().getComponents<const TimeComponent>();
		mDebugData.time = time->getValue();

		for (SDL_Event& sdlEvent : mEngine.getLastFrameEvents())
		{
			ImGui_ImplSDL2_ProcessEvent(&sdlEvent);
		}

		// start the imgui frame
		ImGui_ImplOpenGL2_NewFrame();
		ImGui_ImplSDL2_NewFrame(mEngine.getRawWindow());
		ImGui::NewFrame();

		// update the window hierarchy
		mImguiMainMenu.update(mDebugData);

		// prepare data for rendering to the viewport
		ImGui::Render();
	}

	// schedule rendering on the render thread
	RenderAccessorGameRef renderAccessor = *renderAccessorCmp->getAccessor();
	std::unique_ptr<RenderData> renderData = std::make_unique<RenderData>();
	CustomRenderFunction& syncData = TemplateHelpers::EmplaceVariant<CustomRenderFunction>(renderData->layers);
	syncData.renderThreadFn = [&mutex = mRenderDataMutex]
	{
		std::lock_guard l(mutex);
		ImGui_ImplOpenGL2_NewFrame();
		ImGuiIO& io = ImGui::GetIO();
		glViewport(0, 0, static_cast<int>(io.DisplaySize.x), static_cast<int>(io.DisplaySize.y));
		ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
	};
	renderAccessor.submitData(std::move(renderData));
}

void ImguiSystem::init()
{
	SCOPED_PROFILER("ImguiSystem::initResources");

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	//ImGuiIO& io = ImGui::GetIO();
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// Setup Platform/Renderer bindings
	ImGui_ImplSDL2_InitForOpenGL(mEngine.getRawWindow(), mEngine.getRawGlContext());
	ImGui_ImplOpenGL2_Init();
}

void ImguiSystem::shutdown()
{
	SCOPED_PROFILER("ImguiSystem::shutdown");
	ImGui_ImplOpenGL2_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();
}

#endif // IMGUI_ENABLED
