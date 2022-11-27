#include "Base/precomp.h"

#include "GameLogic/Systems/ResourceStreamingSystem.h"

#include "GameData/World.h"
#include "GameData/GameData.h"
#include "GameData/Components/AnimationClipCreatorComponent.generated.h"
#include "GameData/Components/AnimationClipsComponent.generated.h"
#include "GameData/Components/AnimationGroupCreatorComponent.generated.h"
#include "GameData/Components/AnimationGroupsComponent.generated.h"
#include "GameData/Components/SpriteCreatorComponent.generated.h"
#include "GameData/Components/SpriteRenderComponent.generated.h"
#include "GameData/Components/TileGridComponent.generated.h"
#include "GameData/Components/TileGridCreatorComponent.generated.h"

#include "Utils/ResourceManagement/ResourceManager.h"
#include "Utils/SharedManagers/WorldHolder.h"

#include "HAL/Graphics/Sprite.h"

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
	World& world = mWorldHolder.getWorld();
	EntityManager& entityManager = world.getEntityManager();

#ifndef DEDICATED_SERVER
	// load sprites
	entityManager.forEachComponentSetWithEntity<SpriteCreatorComponent>(
			[this, &entityManager](Entity entity, SpriteCreatorComponent* spriteCreator)
	{
		const auto& descriptions = spriteCreator->getDescriptions();
		Assert(!descriptions.empty(), "Sprite descriptions should not be empty");

		SpriteRenderComponent* spriteRender = entityManager.scheduleAddComponent<SpriteRenderComponent>(entity);
		size_t spritesCount = descriptions.size();
		auto& spriteDatas = spriteRender->getSpriteDatasRef();
		spriteDatas.resize(spritesCount);
		for (size_t i = 0; i < spritesCount; ++i)
		{
			spriteDatas[i].spriteHandle = mResourceManager.lockResource<Graphics::Sprite>(descriptions[i].path);
			spriteDatas[i].params = descriptions[i].params;
			int id = spriteRender->getMaxSpriteId();
			spriteRender->getSpriteIdsRef().push_back(id++);
			spriteRender->setMaxSpriteId(id);
		}
		entityManager.scheduleRemoveComponent<SpriteCreatorComponent>(entity);
	});
	entityManager.executeScheduledActions();

	// load single animations clips
	entityManager.forEachComponentSetWithEntity<AnimationClipCreatorComponent>(
			[this, &entityManager](Entity entity, AnimationClipCreatorComponent* animationClipCreator)
	{
		const auto& descriptions = animationClipCreator->getDescriptionsRef();
		Assert(!descriptions.empty(), "Animation descriptions should not be empty");

		AnimationClipsComponent* animationClips = entityManager.scheduleAddComponent<AnimationClipsComponent>(entity);
		size_t animationCount = descriptions.size();
		auto& animations = animationClips->getDatasRef();
		animations.resize(animationCount);

		auto [spriteRender] = entityManager.getEntityComponents<SpriteRenderComponent>(entity);
		if (spriteRender == nullptr)
		{
			spriteRender = entityManager.scheduleAddComponent<SpriteRenderComponent>(entity);
		}

		auto& spriteDatas = spriteRender->getSpriteDatasRef();
		for (size_t i = 0; i < animationCount; ++i)
		{
			animations[i].animation = mResourceManager.lockResource<Graphics::SpriteAnimationClip>(descriptions[i].path);
			animations[i].params = descriptions[i].params;
			animations[i].sprites = mResourceManager.tryGetResource<Graphics::SpriteAnimationClip>(animations[i].animation)->getSprites();

			AssertFatal(!animations[i].sprites.empty(), "Empty SpriteAnimation '%s'", descriptions[i].path.c_str());
			spriteDatas.emplace_back(descriptions[i].spriteParams, animations[i].sprites.front());
			int id = spriteRender->getMaxSpriteId();
			animations[i].spriteId = id;
			spriteRender->getSpriteIdsRef().push_back(id++);
			spriteRender->setMaxSpriteId(id);

			Assert(spriteRender->getSpriteIds().size() == spriteDatas.size(), "Sprites and SpriteIds have diverged");
		}

		entityManager.scheduleAddComponent<AnimationClipsComponent>(entity);

		entityManager.scheduleRemoveComponent<AnimationClipCreatorComponent>(entity);
	});
	entityManager.executeScheduledActions();

	// load animation groups
	entityManager.forEachComponentSetWithEntity<AnimationGroupCreatorComponent>(
			[this, &entityManager](Entity entity, AnimationGroupCreatorComponent* animationGroupCreator)
	{
		AnimationGroupsComponent* animationGroups = entityManager.scheduleAddComponent<AnimationGroupsComponent>(entity);

		auto [animationClips] = entityManager.getEntityComponents<AnimationClipsComponent>(entity);
		if (animationClips == nullptr)
		{
			animationClips = entityManager.scheduleAddComponent<AnimationClipsComponent>(entity);
		}
		auto& clipDatas = animationClips->getDatasRef();

		auto [spriteRender] = entityManager.getEntityComponents<SpriteRenderComponent>(entity);
		if (spriteRender == nullptr)
		{
			spriteRender = entityManager.scheduleAddComponent<SpriteRenderComponent>(entity);
		}
		auto& spriteDatas = spriteRender->getSpriteDatasRef();

		const size_t animGroupCount = animationGroupCreator->getAnimationGroups().size();
		for (size_t i = 0; i < animGroupCount; ++i)
		{
			ResourceHandle animGroupHandle = mResourceManager.lockResource<Graphics::AnimationGroup>(animationGroupCreator->getAnimationGroups()[i]);

			const Graphics::AnimationGroup* group = mResourceManager.tryGetResource<Graphics::AnimationGroup>(animGroupHandle);
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

		entityManager.scheduleRemoveComponent<AnimationGroupCreatorComponent>(entity);
	});
	entityManager.executeScheduledActions();
#endif // !DEDICATED_SERVER

	// load tile grids
	entityManager.forEachComponentSetWithEntity<TileGridCreatorComponent>(
			[this, &entityManager](Entity entity, TileGridCreatorComponent* tileGridCreator)
	{
		TileGridComponent* tileGrid = entityManager.scheduleAddComponent<TileGridComponent>(entity);

		ResourceHandle tileGridHandle = mResourceManager.lockResource<Graphics::TileGrid>(ResourcePath(tileGridCreator->getGridPath()));
		const Graphics::TileGrid* tileGridResource = mResourceManager.tryGetResource<Graphics::TileGrid>(tileGridHandle);
		if (tileGridResource == nullptr) {
			return;
		}

		tileGrid->setGridData(tileGridResource->getTileGridParams());
		tileGrid->setSize(tileGridCreator->getSize());

		entityManager.scheduleRemoveComponent<TileGridCreatorComponent>(entity);
	});
	entityManager.executeScheduledActions();
}
