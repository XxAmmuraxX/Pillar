#pragma once

#include "Pillar/ECS/Systems/System.h"
#include "Pillar/ECS/Components/Rendering/AnimationClip.h"
#include <entt/entt.hpp>
#include <unordered_map>
#include <string>
#include <memory>

namespace Pillar {

	// Forward declarations
	struct AnimationComponent;

	/**
	 * @brief System that updates animation playback and manages animation clips
	 * 
	 * Responsibilities:
	 * - Load and store animation clips from JSON files
	 * - Update AnimationComponent state each frame
	 * - Advance animation frames based on time
	 * - Update SpriteComponent texture coordinates for current frame
	 * - Fire animation events when reaching specific frames
	 * 
	 * The system queries all entities with both AnimationComponent and SpriteComponent,
	 * then updates their sprite rendering based on the current animation frame.
	 */
	class AnimationSystem : public System
	{
	public:
		AnimationSystem() = default;
		virtual ~AnimationSystem() = default;

		void OnAttach(Scene* scene) override;
		void OnDetach() override;
		void OnUpdate(float deltaTime) override;

		/**
		 * @brief Load an animation clip from a JSON file
		 * @param filePath Path to the .anim.json file
		 * @return True if loaded successfully, false otherwise
		 */
		bool LoadAnimationClip(const std::string& filePath);

		/**
		 * @brief Register an animation clip programmatically
		 * @param clip The animation clip to register
		 */
		void RegisterClip(const AnimationClip& clip);

		/**
		 * @brief Get a loaded animation clip by name
		 * @param name Name of the animation clip
		 * @return Pointer to clip if found, nullptr otherwise
		 */
		AnimationClip* GetClip(const std::string& name);

		/**
		 * @brief Check if a clip with the given name exists
		 */
		bool HasClip(const std::string& name) const;

		/**
		 * @brief Get the number of loaded animation clips
		 */
		size_t GetClipCount() const { return m_AnimationLibrary.size(); }

		/**
		 * @brief Get all loaded animation clips (for editor iteration)
		 * @return Reference to the clip library map
		 */
		const std::unordered_map<std::string, AnimationClip>& GetAllClips() const { return m_AnimationLibrary; }

		/**
		 * @brief Update animation for a specific entity (for edit-mode preview)
		 * @param entity The entity to update
		 * @param dt Delta time
		 * 
		 * This method allows updating a single entity's animation in edit mode
		 * without requiring the Playing flag to be true.
		 */
		void UpdateInEditMode(entt::entity entity, float dt);

		/**
		 * @brief Unload a specific animation clip
		 * @param name Name of the clip to unload
		 * @return True if clip was found and unloaded
		 */
		bool UnloadClip(const std::string& name);

		/**
		 * @brief Clear all loaded animation clips
		 */
		void ClearLibrary();

	private:
		std::unordered_map<std::string, AnimationClip> m_AnimationLibrary;

		/**
		 * @brief Update a single entity's animation
		 */
		void UpdateAnimation(entt::entity entity, float dt);

	/**
	 * @brief Advance to the next frame in the animation
	 */
	void AdvanceFrame(AnimationComponent& anim, AnimationClip& clip, entt::entity entity);		/**
		 * @brief Update sprite component UVs and texture from animation frame
		 */
		void UpdateSpriteFromFrame(entt::entity entity, const AnimationFrame& frame);

		/**
		 * @brief Fire animation events for the current frame transition
		 */
		void FireAnimationEvents(AnimationComponent& anim, AnimationClip& clip, 
			int oldFrame, entt::entity entity);
	};

} // namespace Pillar
