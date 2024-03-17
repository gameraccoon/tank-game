#include "Base/precomp.h"

#include "GameLogic/Systems/ResourceStreamingSystem.h"

#include "GameData/Components/AnimationClipCreatorComponent.generated.h"
#include "GameData/Components/AnimationClipsComponent.generated.h"
#include "GameData/Components/AnimationGroupCreatorComponent.generated.h"
#include "GameData/Components/AnimationGroupsComponent.generated.h"
#include "GameData/Components/SpriteCreatorComponent.generated.h"
#include "GameData/Components/SpriteRenderComponent.generated.h"
#include "GameData/Components/TileGridComponent.generated.h"
#include "GameData/Components/TileGridCreatorComponent.generated.h"
#include "GameData/GameData.h"
#include "GameData/WorldLayer.h"

#include "HAL/Graphics/Sprite.h"

#include "Utils/ResourceManagement/ResourceManager.h"
#include "Utils/SharedManagers/WorldHolder.h"

#include "GameLogic/Resources/AnimationGroup.h"
#include "GameLogic/Resources/SpriteAnimationClip.h"
#include "GameLogic/Resources/TileGrid.h"

ResourceStreamingSystem::ResourceStreamingSystem(
		WorldHolder& worldHolder,
		ResourceManager& resourceManager) noexcept
	: mWorldHolder(worldHolder)
	, mResourceManager(resourceManager)
{
}

void ResourceStreamingSystem::update()
{
	SCOPED_PROFILER("ResourceStreamingSystem::update");
	CombinedEntityManagerView& entityManager = mWorldHolder.getFullWorld();

#ifndef DISABLE_SDL
	// load sprites
	entityManager.forEachComponentSetWithEntity<SpriteCreatorComponent>(
			[this](EntityView entity, SpriteCreatorComponent* spriteCreator)
	{
		const auto& descriptions = spriteCreator->getDescriptions();
		Assert(!descriptions.empty(), "Sprite descriptions should not be empty");

		SpriteRenderComponent* spriteRender = entity.scheduleAddComponent<SpriteRenderComponent>();
		size_t spritesCount = descriptions.size();
		auto& spriteDatas = spriteRender->getSpriteDatasRef();
		spriteDatas.resize(spritesCount);
		for (size_t i = 0; i < spritesCount; ++i)
		{
			spriteDatas[i].spriteHandle = mResourceManager.lockResource<HAL::Graphics::Sprite>(descriptions[i].path);
			spriteDatas[i].params = descriptions[i].params;
			int id = spriteRender->getMaxSpriteId();
			spriteRender->getSpriteIdsRef().push_back(id++);
			spriteRender->setMaxSpriteId(id);
		}
		entity.scheduleRemoveComponent<SpriteCreatorComponent>();
	});
	entityManager.executeScheduledActions();

	// load single animations clips
	entityManager.forEachComponentSetWithEntity<AnimationClipCreatorComponent>(
			[this](EntityView entity, AnimationClipCreatorComponent* animationClipCreator)
	{
		const auto& descriptions = animationClipCreator->getDescriptionsRef();
		Assert(!descriptions.empty(), "Animation descriptions should not be empty");

		AnimationClipsComponent* animationClips = entity.scheduleAddComponent<AnimationClipsComponent>();
		size_t animationCount = descriptions.size();
		auto& animations = animationClips->getDatasRef();
		animations.resize(animationCount);

		auto [spriteRender] = entity.getComponents<SpriteRenderComponent>();
		if (spriteRender == nullptr)
		{
			spriteRender = entity.scheduleAddComponent<SpriteRenderComponent>();
		}

		auto& spriteDatas = spriteRender->getSpriteDatasRef();
		for (size_t i = 0; i < animationCount; ++i)
		{
			animations[i].animation = mResourceManager.lockResource<HAL::Graphics::SpriteAnimationClip>(descriptions[i].path);
			animations[i].params = descriptions[i].params;
			animations[i].sprites = mResourceManager.tryGetResource<HAL::Graphics::SpriteAnimationClip>(animations[i].animation)->getSprites();

			AssertFatal(!animations[i].sprites.empty(), "Empty SpriteAnimation '%s'", descriptions[i].path.getRelativePath().c_str());
			spriteDatas.emplace_back(descriptions[i].spriteParams, animations[i].sprites.front());
			int id = spriteRender->getMaxSpriteId();
			animations[i].spriteId = id;
			spriteRender->getSpriteIdsRef().push_back(id++);
			spriteRender->setMaxSpriteId(id);

			Assert(spriteRender->getSpriteIds().size() == spriteDatas.size(), "Sprites and SpriteIds have diverged");
		}

		entity.scheduleAddComponent<AnimationClipsComponent>();

		entity.scheduleRemoveComponent<AnimationClipCreatorComponent>();
	});
	entityManager.executeScheduledActions();

	// load animation groups
	entityManager.forEachComponentSetWithEntity<AnimationGroupCreatorComponent>(
			[this](EntityView entity, AnimationGroupCreatorComponent* animationGroupCreator)
	{
		AnimationGroupsComponent* animationGroups = entity.scheduleAddComponent<AnimationGroupsComponent>();

		auto [animationClips] = entity.getComponents<AnimationClipsComponent>();
		if (animationClips == nullptr)
		{
			animationClips = entity.scheduleAddComponent<AnimationClipsComponent>();
		}
		auto& clipDatas = animationClips->getDatasRef();

		auto [spriteRender] = entity.getComponents<SpriteRenderComponent>();
		if (spriteRender == nullptr)
		{
			spriteRender = entity.scheduleAddComponent<SpriteRenderComponent>();
		}
		auto& spriteDatas = spriteRender->getSpriteDatasRef();

		const size_t animGroupCount = animationGroupCreator->getAnimationGroups().size();
		for (size_t i = 0; i < animGroupCount; ++i)
		{
			ResourceHandle animGroupHandle = mResourceManager.lockResource<HAL::Graphics::AnimationGroup>(animationGroupCreator->getAnimationGroups()[i]);

			const HAL::Graphics::AnimationGroup* group = mResourceManager.tryGetResource<HAL::Graphics::AnimationGroup>(animGroupHandle);
			AnimationGroup<StringId> animationGroup;
			animationGroup.currentState = group->getDefaultState();
			animationGroup.animationClips = group->getAnimationClips();
			animationGroup.stateMachineId = group->getStateMachineId();
			animationGroup.animationClipIdx = clipDatas.size();

			AnimationClip clip;
			clip.params = animationGroupCreator->getClipParams()[i];
			clip.sprites = animationGroup.animationClips.find(animationGroup.currentState)->second;

			animationGroups->getDataRef().emplace_back(std::move(animationGroup));

			spriteDatas.emplace_back(animationGroupCreator->getSpriteParams()[i], clip.sprites.front());

			int id = spriteRender->getMaxSpriteId();
			clip.spriteId = id;
			spriteRender->getSpriteIdsRef().push_back(id++);
			spriteRender->setMaxSpriteId(id);

			clipDatas.emplace_back(std::move(clip));
		}

		entity.scheduleRemoveComponent<AnimationGroupCreatorComponent>();
	});
	entityManager.executeScheduledActions();
#endif // !DISABLE_SDL

	// load tile grids
	entityManager.forEachComponentSetWithEntity<TileGridCreatorComponent>(
			[this](EntityView entity, TileGridCreatorComponent* tileGridCreator)
	{
		TileGridComponent* tileGrid = entity.scheduleAddComponent<TileGridComponent>();

		ResourceHandle tileGridHandle = mResourceManager.lockResource<HAL::Graphics::TileGrid>(mResourceManager.getAbsoluteResourcePath(tileGridCreator->getGridPath()));
		const HAL::Graphics::TileGrid* tileGridResource = mResourceManager.tryGetResource<HAL::Graphics::TileGrid>(tileGridHandle);
		if (tileGridResource == nullptr) {
			return;
		}

		tileGrid->setGridData(tileGridResource->getTileGridParams());
		tileGrid->setSize(tileGridCreator->getSize());

		entity.scheduleRemoveComponent<TileGridCreatorComponent>();
	});
	entityManager.executeScheduledActions();
}
