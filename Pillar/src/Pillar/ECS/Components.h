#pragma once

/**
 * @file Components.h
 * @brief Umbrella header that includes all ECS components
 * 
 * Include this single header to access all Pillar ECS components:
 * - Core: Transform, Tag, UUID, Hierarchy
 * - Physics: Rigidbody, Collider, Velocity
 * - Rendering: Sprite, Camera, Animation, Lighting
 * - Gameplay: Bullet, XPGem, Particles
 * - Audio: AudioSource, AudioListener
 */

// Core components
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/ECS/Components/Core/TagComponent.h"
#include "Pillar/ECS/Components/Core/UUIDComponent.h"
#include "Pillar/ECS/Components/Core/HierarchyComponent.h"

// Physics components
#include "Pillar/ECS/Components/Physics/RigidbodyComponent.h"
#include "Pillar/ECS/Components/Physics/ColliderComponent.h"
#include "Pillar/ECS/Components/Physics/VelocityComponent.h"

// Rendering components
#include "Pillar/ECS/Components/Rendering/SpriteComponent.h"
#include "Pillar/ECS/Components/Rendering/CameraComponent.h"
#include "Pillar/ECS/Components/Rendering/AnimationComponent.h"
#include "Pillar/ECS/Components/Rendering/AnimationClip.h"
#include "Pillar/ECS/Components/Rendering/AnimationClipBuilder.h"
#include "Pillar/ECS/Components/Rendering/AnimationFrame.h"
#include "Pillar/ECS/Components/Rendering/Light2DComponent.h"
#include "Pillar/ECS/Components/Rendering/ShadowCaster2DComponent.h"

// Gameplay components
#include "Pillar/ECS/Components/Gameplay/BulletComponent.h"
#include "Pillar/ECS/Components/Gameplay/HealthComponent.h"
#include "Pillar/ECS/Components/Gameplay/XPGemComponent.h"
#include "Pillar/ECS/Components/Gameplay/ParticleComponent.h"
#include "Pillar/ECS/Components/Gameplay/ParticleEmitterComponent.h"
#include "Pillar/ECS/Components/Gameplay/ParticleAnimationCurves.h"

// Audio components
#include "Pillar/ECS/Components/Audio/AudioSourceComponent.h"
#include "Pillar/ECS/Components/Audio/AudioListenerComponent.h"
