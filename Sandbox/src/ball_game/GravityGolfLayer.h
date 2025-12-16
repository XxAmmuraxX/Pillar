#pragma once

#include "Pillar.h"
#include "Pillar/Renderer/Renderer2DBackend.h"
#include "Pillar/ECS/Scene.h"
#include "Pillar/ECS/Entity.h"
#include "Pillar/ECS/SceneSerializer.h"
#include "Pillar/ECS/ComponentRegistry.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/ECS/Components/Core/TagComponent.h"
#include "Pillar/ECS/Components/Physics/RigidbodyComponent.h"
#include "Pillar/ECS/Components/Physics/ColliderComponent.h"
#include "Pillar/ECS/Systems/PhysicsSystem.h"
#include "Pillar/ECS/Systems/PhysicsSyncSystem.h"
#include "Pillar/Utils/AssetManager.h"

#include "GameComponents.h"
#include "TutorialLevels.h"
#include "GameAudio.h"

#include <imgui.h>
#include <memory>
#include <vector>
#include <cmath>
#include <functional>
#include <algorithm>
#include <cstdlib>
#include <glm/gtc/constants.hpp>
#include <limits>
#include <box2d/box2d.h>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <string>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <cctype>
#include <nlohmann/json.hpp>

namespace BallGame {

    // Custom contact listener to play wall-hit sounds when the ball collides with walls/platforms
    class BallContactListener : public b2ContactListener
    {
    public:
        BallContactListener(Pillar::Scene* scene, GameAudio* audio, float* timePtr)
            : m_Scene(scene), m_Audio(audio), m_TimePtr(timePtr) {}

        void BeginContact(b2Contact* contact) override { Handle(contact, nullptr); }

        void PostSolve(b2Contact* contact, const b2ContactImpulse* impulse) override
        {
            Handle(contact, impulse);
        }

    private:
        Pillar::Entity ToEntity(b2Fixture* fixture) const
        {
            if (!fixture)
                return {};
            b2Body* body = fixture->GetBody();
            if (!body)
                return {};

            uint32_t id = static_cast<uint32_t>(body->GetUserData().pointer);
            entt::entity enttId = static_cast<entt::entity>(id);
            if (!m_Scene || !m_Scene->GetRegistry().valid(enttId))
                return {};
            return Pillar::Entity(enttId, m_Scene);
        }

        void Handle(b2Contact* contact, const b2ContactImpulse* impulse)
        {
            if (!m_Scene || !m_Audio || !contact)
                return;

            Pillar::Entity a = ToEntity(contact->GetFixtureA());
            Pillar::Entity b = ToEntity(contact->GetFixtureB());
            if (!a && !b)
                return;

            const bool hasBall = (a && a.HasComponent<GolfBallComponent>()) || (b && b.HasComponent<GolfBallComponent>());
            if (!hasBall)
                return;

            const bool hitWall = (a && a.HasComponent<WallComponent>()) || (b && b.HasComponent<WallComponent>());
            if (!hitWall)
                return;

            // Ignore very soft contacts
            if (impulse && impulse->count > 0)
            {
                float normalImpulse = impulse->normalImpulses[0];
                if (normalImpulse < 0.05f)
                    return;
            }

            const float now = m_TimePtr ? *m_TimePtr : 0.0f;
            if (now - m_LastPlayTime < 0.08f)
                return; // Rate-limit to avoid chatter

            m_LastPlayTime = now;
            m_Audio->PlayBounce();
        }

        Pillar::Scene* m_Scene = nullptr;
        GameAudio* m_Audio = nullptr;
        float* m_TimePtr = nullptr;
        float m_LastPlayTime = -1.0f;
    };

    // Z-layer constants for proper render ordering (lower = further back)
    namespace ZLayer
    {
        constexpr float Background = -0.5f;
        constexpr float Goal = -0.2f;
        constexpr float WallShadow = -0.15f;
        constexpr float Wall = -0.1f;
        constexpr float BoostPad = -0.07f;
        constexpr float GravityWell = -0.05f;
        constexpr float BallShadow = 0.05f;
        constexpr float Particles = 0.08f;
        constexpr float Ball = 0.1f;
        constexpr float AimLine = 0.2f;
        constexpr float UI = 0.5f;
    }

    class GravityGolfLayer : public Pillar::Layer
    {
    public:
        GravityGolfLayer()
            : Layer("GravityGolfLayer"),
              m_CameraController(16.0f / 9.0f, false) // No rotation
        {
        }

        void OnAttach() override
        {
            Layer::OnAttach();
            PIL_INFO("Gravity Golf Layer attached!");

            // Initialize audio
            m_Audio.Init();

            // Ensure BallGame components participate in scene serialization
            RegisterBallGameComponents();

            // Load textures (must exist in Sandbox/assets/textures)
            m_TextureGrass = Pillar::Texture2D::Create("textures/grass_tile.png");
            m_TextureWall = Pillar::Texture2D::Create("textures/wall.png");
            m_TextureGoal = Pillar::Texture2D::Create("textures/hole.png");
            m_TextureBall = Pillar::Texture2D::Create("textures/golf_ball.png");
            m_TextureBooster = Pillar::Texture2D::Create("textures/booster.png");
            m_TextureIcons = Pillar::Texture2D::Create("textures/icons.png");

            // Log texture loading status
            PIL_INFO("Texture loading status:");
            PIL_INFO("  Grass: {}", m_TextureGrass ? "OK" : "FAILED");
            PIL_INFO("  Wall: {}", m_TextureWall ? "OK" : "FAILED");
            PIL_INFO("  Goal: {}", m_TextureGoal ? "OK" : "FAILED");
            PIL_INFO("  Ball: {}", m_TextureBall ? "OK" : "FAILED");
            PIL_INFO("  Booster: {}", m_TextureBooster ? "OK" : "FAILED");
            PIL_INFO("  Icons: {}", m_TextureIcons ? "OK" : "FAILED");

            EnsureUIStyle();

            // Load all tutorial levels
            m_Levels = TutorialLevels::GetAllLevels();
            m_BestShots.assign(m_Levels.size(), std::numeric_limits<int>::max());

            // Start at the main menu
            m_GameState = GameState::MainMenu;
            m_ShowMainMenu = true;

            // Set camera to max zoom out for main menu
            m_CameraController.SetZoomLevel(8.5f);

            // Initialize menu particles for animated background
            InitMenuParticles();

            PIL_INFO("Gravity Golf initialized with {} tutorial levels!", m_Levels.size());
        }

        void OnDetach() override
        {
            CleanupLevel();
            m_Audio.StopMusic();
            Layer::OnDetach();
        }

        void OnUpdate(float dt) override
        {
            m_Time += dt;
            m_MenuAnimTime += dt;
            m_Audio.EnsureMusicPlaying();

            // Handle main menu state separately
            if (m_GameState == GameState::MainMenu)
            {
                UpdateMenuParticles(dt);
                RenderMenuBackground();
                return;
            }

            // Handle restart key
            if (Pillar::Input::IsKeyPressed(PIL_KEY_R))
            {
                if (!m_RestartKeyHeld)
                {
                    m_RestartKeyHeld = true;
                    RestartLevel();
                }
            }
            else
            {
                m_RestartKeyHeld = false;
            }

            // Update camera
            m_CameraController.OnUpdate(dt);

            // Update physics
            if (m_PhysicsSystem && m_GameState != GameState::Paused)
            {
                UpdateMovingPlatforms(dt);
                m_PhysicsSystem->OnUpdate(dt);
                m_PhysicsSyncSystem->OnUpdate(dt);
            }

            // Update game state
            UpdateGameState(dt);

            // Animate particles after physics
            UpdateParticles(dt);

            // Render
            Render();
        }

        void OnEvent(Pillar::Event& event) override
        {
            // Don't process camera events in main menu
            if (m_GameState != GameState::MainMenu)
                m_CameraController.OnEvent(event);

            Pillar::EventDispatcher dispatcher(event);
            dispatcher.Dispatch<Pillar::MouseButtonPressedEvent>(
                [this](Pillar::MouseButtonPressedEvent& e) -> bool {
                    return OnMouseButtonPressed(e);
                });
            dispatcher.Dispatch<Pillar::KeyPressedEvent>(
                [this](Pillar::KeyPressedEvent& e) -> bool {
                    return OnKeyPressed(e);
                });
        }

        void OnImGuiRender() override
        {
            RenderUI();
        }

    private:
        // ============================================
        // Key handling
        // ============================================
        bool OnKeyPressed(Pillar::KeyPressedEvent& e)
        {
            if (e.GetKeyCode() == PIL_KEY_ESCAPE)
            {
                // Close level select if open
                if (m_ShowLevelSelect)
                {
                    m_ShowLevelSelect = false;
                    return true;
                }
                // Return to main menu from game
                if (m_GameState != GameState::MainMenu)
                {
                    ReturnToMainMenu();
                    return true;
                }
            }
            return false;
        }

        // ============================================
        // Level Management
        // ============================================
        void LoadLevel(int levelIndex)
        {
            if (levelIndex < 0 || levelIndex >= static_cast<int>(m_Levels.size()))
                return;

            CleanupLevel();

            m_CurrentLevelIndex = levelIndex;
            const LevelData& level = m_Levels[levelIndex];
            const std::string scenePath = BuildLevelScenePath(level);      // relative to assets/
            const std::filesystem::path fullScenePath = ResolveScenePath(scenePath);

            // Create scene
            m_Scene = std::make_unique<Pillar::Scene>();

            // Create physics (no gravity - top down view)
            m_PhysicsSystem = new Pillar::PhysicsSystem(glm::vec2(0.0f, 0.0f));
            m_PhysicsSyncSystem = new Pillar::PhysicsSyncSystem();

            m_PhysicsSystem->OnAttach(m_Scene.get());
            m_PhysicsSyncSystem->OnAttach(m_Scene.get());
            m_Scene->SetPhysicsSystem(m_PhysicsSystem);

            // Replace default listener with game-specific one (wall hit sounds)
            m_ContactListener = std::make_unique<BallContactListener>(m_Scene.get(), &m_Audio, &m_Time);
            if (auto* world = m_PhysicsSystem->GetWorld())
                world->SetContactListener(m_ContactListener.get());

            bool loadedFromJson = false;
            if (std::filesystem::exists(fullScenePath))
            {
                Pillar::SceneSerializer serializer(m_Scene.get());
                loadedFromJson = serializer.Deserialize(scenePath);
                if (!loadedFromJson)
                    PIL_WARN("Failed to deserialize scene '{}', regenerating from procedural definition.", fullScenePath.string());
            }

            if (!loadedFromJson)
            {
                // Create game objects procedurally, then persist to JSON for next run
                CreateBall(level.BallStart);
                CreateGoal(level.GoalPosition, level.Par);
                CreateWalls(level.Walls);
                CreateGravityWells(level.GravityWells);
                CreateBoostPads(level.BoostPads);
                CreateMovingPlatforms(level.MovingPlatforms);

                Pillar::SceneSerializer serializer(m_Scene.get());
                if (serializer.Serialize(scenePath))
                    PIL_INFO("Serialized level {} to '{}'.", level.LevelNumber, fullScenePath.string());
                else
                    PIL_WARN("Failed to serialize level {} to '{}'.", level.LevelNumber, fullScenePath.string());
            }

            RebindRuntimeEntities();

            // Reset state
            m_GameState = GameState::Aiming;
            m_ShotCount = 0;
            m_LevelCompleteTimer = 0.0f;
            m_ShowLevelComplete = false;
            m_BallHidden = false;
            m_Particles.clear();
            m_TrailAccumulator = 0.0f;
            m_WellPulseAccumulator = 0.0f;
            for (auto& pad : m_BoostPads)
                pad.Entity.GetComponent<BoostPadComponent>().Cooldown = 0.0f;

            PIL_INFO("Loaded level {} ({})", level.LevelNumber, loadedFromJson ? "deserialized" : "procedural + saved");
        }

        void CleanupLevel()
        {
            if (m_PhysicsSystem)
            {
                m_PhysicsSystem->OnDetach();
                delete m_PhysicsSystem;
                m_PhysicsSystem = nullptr;
            }
            if (m_PhysicsSyncSystem)
            {
                m_PhysicsSyncSystem->OnDetach();
                delete m_PhysicsSyncSystem;
                m_PhysicsSyncSystem = nullptr;
            }
            m_ContactListener.reset();
            m_Scene.reset();
            m_BallEntity = {};
            m_GoalEntity = {};
            m_GravityWells.clear();
            m_BoostPads.clear();
            m_MovingPlatforms.clear();
            m_Particles.clear();
        }

        void RestartLevel()
        {
            LoadLevel(m_CurrentLevelIndex);
        }

        void ToggleMute()
        {
            if (m_IsMuted)
            {
                m_IsMuted = false;
                m_Audio.SetMasterVolume(m_LastVolume);
            }
            else
            {
                m_IsMuted = true;
                m_LastVolume = m_Audio.GetMasterVolume();
                m_Audio.SetMasterVolume(0.0f);
            }
        }

        void NextLevel()
        {
            int nextIndex = m_CurrentLevelIndex + 1;
            if (nextIndex < static_cast<int>(m_Levels.size()))
            {
                LoadLevel(nextIndex);
            }
            else
            {
                // Completed all levels - restart from beginning
                LoadLevel(0);
            }

            m_ShowLevelSelect = false;
        }

        std::string BuildLevelScenePath(const LevelData& level) const
        {
            std::string name = level.Name ? level.Name : "level";
            std::string slug;
            slug.reserve(name.size());
            for (char c : name)
            {
                unsigned char uc = static_cast<unsigned char>(c);
                if (std::isalnum(uc))
                {
                    slug.push_back(static_cast<char>(std::tolower(uc)));
                }
                else if (c == ' ' || c == '-' || c == '_')
                {
                    if (!slug.empty() && slug.back() != '_')
                        slug.push_back('_');
                }
            }
            if (slug.empty())
                slug = "level";

            std::ostringstream oss;
            oss << std::setfill('0') << std::setw(2) << level.LevelNumber << "_" << slug << ".scene.json";

            std::filesystem::path rel = std::filesystem::path("scenes") / "gravity_golf" / oss.str();
            return rel.generic_string();
        }

        std::filesystem::path ResolveScenePath(const std::string& relativePath) const
        {
            std::filesystem::path path(relativePath);
            if (path.is_absolute())
                return path;

            std::filesystem::path assetsDir = Pillar::AssetManager::GetAssetsDirectory();
            if (assetsDir.empty())
                return path;

            return assetsDir / path;
        }

        void RebindRuntimeEntities()
        {
            m_BallEntity = {};
            m_GoalEntity = {};
            m_GravityWells.clear();
            m_BoostPads.clear();
            m_MovingPlatforms.clear();

            if (!m_Scene)
                return;

            auto& registry = m_Scene->GetRegistry();
            auto view = registry.view<Pillar::TagComponent>();
            for (auto entityHandle : view)
            {
                Pillar::Entity e(entityHandle, m_Scene.get());
                if (e.HasComponent<GolfBallComponent>())
                    m_BallEntity = e;
                if (e.HasComponent<GoalComponent>())
                    m_GoalEntity = e;
                if (e.HasComponent<GravityWellComponent>())
                    m_GravityWells.push_back(e);
                if (e.HasComponent<BoostPadComponent>())
                    m_BoostPads.push_back({e});
                if (e.HasComponent<MovingPlatformComponent>())
                    m_MovingPlatforms.push_back({e});
            }
        }

        void RegisterBallGameComponents()
        {
            using json = nlohmann::json;

            auto& registry = Pillar::ComponentRegistry::Get();
            registry.EnsureBuiltinsRegistered();

            auto vec2ToJson = [](const glm::vec2& v) { return json::array({v.x, v.y}); };
            auto vec4ToJson = [](const glm::vec4& v) { return json::array({v.x, v.y, v.z, v.w}); };
            auto jsonToVec2 = [](const json& j) { return glm::vec2(j[0].get<float>(), j[1].get<float>()); };
            auto jsonToVec4 = [](const json& j) { return glm::vec4(j[0].get<float>(), j[1].get<float>(), j[2].get<float>(), j[3].get<float>()); };

            if (!registry.IsRegistered<GolfBallComponent>())
            {
                registry.Register<GolfBallComponent>(
                    "golfBall",
                    [vec2ToJson](Pillar::Entity e) -> json {
                        if (!e.HasComponent<GolfBallComponent>()) return nullptr;
                        auto& c = e.GetComponent<GolfBallComponent>();
                        return json{
                            {"shotCount", c.ShotCount},
                            {"maxPower", c.MaxPower},
                            {"minPower", c.MinPower},
                            {"isMoving", c.IsMoving},
                            {"inGoal", c.InGoal},
                            {"lastShotPos", vec2ToJson(c.LastShotPosition)}
                        };
                    },
                    [jsonToVec2](Pillar::Entity e, const json& j) {
                        auto& c = e.HasComponent<GolfBallComponent>() ? e.GetComponent<GolfBallComponent>() : e.AddComponent<GolfBallComponent>();
                        c.ShotCount = j.value("shotCount", 0);
                        c.MaxPower = j.value("maxPower", 12.0f);
                        c.MinPower = j.value("minPower", 2.0f);
                        c.IsMoving = j.value("isMoving", false);
                        c.InGoal = j.value("inGoal", false);
                        if (j.contains("lastShotPos") && j["lastShotPos"].is_array())
                            c.LastShotPosition = jsonToVec2(j["lastShotPos"]);
                    },
                    [](Pillar::Entity src, Pillar::Entity dst) {
                        if (!src.HasComponent<GolfBallComponent>()) return;
                        auto& s = src.GetComponent<GolfBallComponent>();
                        auto& d = dst.AddComponent<GolfBallComponent>();
                        d = s;
                    });
            }

            if (!registry.IsRegistered<GoalComponent>())
            {
                registry.Register<GoalComponent>(
                    "goal",
                    [](Pillar::Entity e) -> json {
                        if (!e.HasComponent<GoalComponent>()) return nullptr;
                        auto& c = e.GetComponent<GoalComponent>();
                        return json{
                            {"captureRadius", c.CaptureRadius},
                            {"captureSpeed", c.CaptureSpeed},
                            {"par", c.ParScore},
                            {"captured", c.Captured}
                        };
                    },
                    [](Pillar::Entity e, const json& j) {
                        auto& c = e.HasComponent<GoalComponent>() ? e.GetComponent<GoalComponent>() : e.AddComponent<GoalComponent>();
                        c.CaptureRadius = j.value("captureRadius", 0.6f);
                        c.CaptureSpeed = j.value("captureSpeed", 2.5f);
                        c.ParScore = j.value("par", 3);
                        c.Captured = j.value("captured", false);
                    },
                    [](Pillar::Entity src, Pillar::Entity dst) {
                        if (!src.HasComponent<GoalComponent>()) return;
                        auto& s = src.GetComponent<GoalComponent>();
                        auto& d = dst.AddComponent<GoalComponent>();
                        d = s;
                    });
            }

            if (!registry.IsRegistered<WallComponent>())
            {
                registry.Register<WallComponent>(
                    "wall",
                    [vec4ToJson](Pillar::Entity e) -> json {
                        if (!e.HasComponent<WallComponent>()) return nullptr;
                        auto& c = e.GetComponent<WallComponent>();
                        return json{
                            {"visible", c.IsVisible},
                            {"color", vec4ToJson(c.Color)}
                        };
                    },
                    [jsonToVec4](Pillar::Entity e, const json& j) {
                        auto& c = e.HasComponent<WallComponent>() ? e.GetComponent<WallComponent>() : e.AddComponent<WallComponent>();
                        c.IsVisible = j.value("visible", true);
                        if (j.contains("color") && j["color"].is_array())
                            c.Color = jsonToVec4(j["color"]);
                    },
                    [](Pillar::Entity src, Pillar::Entity dst) {
                        if (!src.HasComponent<WallComponent>()) return;
                        auto& s = src.GetComponent<WallComponent>();
                        auto& d = dst.AddComponent<WallComponent>();
                        d = s;
                    });
            }

            if (!registry.IsRegistered<GravityWellComponent>())
            {
                registry.Register<GravityWellComponent>(
                    "gravityWell",
                    [](Pillar::Entity e) -> json {
                        if (!e.HasComponent<GravityWellComponent>()) return nullptr;
                        auto& c = e.GetComponent<GravityWellComponent>();
                        return json{{"radius", c.Radius}, {"strength", c.Strength}, {"repulsor", c.IsRepulsor}};
                    },
                    [](Pillar::Entity e, const json& j) {
                        auto& c = e.HasComponent<GravityWellComponent>() ? e.GetComponent<GravityWellComponent>() : e.AddComponent<GravityWellComponent>();
                        c.Radius = j.value("radius", 4.0f);
                        c.Strength = j.value("strength", 22.0f);
                        c.IsRepulsor = j.value("repulsor", false);
                    },
                    [](Pillar::Entity src, Pillar::Entity dst) {
                        if (!src.HasComponent<GravityWellComponent>()) return;
                        auto& s = src.GetComponent<GravityWellComponent>();
                        auto& d = dst.AddComponent<GravityWellComponent>();
                        d = s;
                    });
            }

            if (!registry.IsRegistered<BoostPadComponent>())
            {
                registry.Register<BoostPadComponent>(
                    "boostPad",
                    [vec2ToJson](Pillar::Entity e) -> json {
                        if (!e.HasComponent<BoostPadComponent>()) return nullptr;
                        auto& c = e.GetComponent<BoostPadComponent>();
                        return json{
                            {"size", vec2ToJson(c.Size)},
                            {"direction", vec2ToJson(c.Direction)},
                            {"strength", c.Strength}
                        };
                    },
                    [jsonToVec2](Pillar::Entity e, const json& j) {
                        auto& c = e.HasComponent<BoostPadComponent>() ? e.GetComponent<BoostPadComponent>() : e.AddComponent<BoostPadComponent>();
                        if (j.contains("size") && j["size"].is_array())
                            c.Size = jsonToVec2(j["size"]);
                        if (j.contains("direction") && j["direction"].is_array())
                            c.Direction = jsonToVec2(j["direction"]);
                        c.Strength = j.value("strength", 11.0f);
                        c.Cooldown = 0.0f;
                    },
                    [](Pillar::Entity src, Pillar::Entity dst) {
                        if (!src.HasComponent<BoostPadComponent>()) return;
                        auto& s = src.GetComponent<BoostPadComponent>();
                        auto& d = dst.AddComponent<BoostPadComponent>();
                        d = s;
                        d.Cooldown = 0.0f;
                    });
            }

            if (!registry.IsRegistered<MovingPlatformComponent>())
            {
                registry.Register<MovingPlatformComponent>(
                    "movingPlatform",
                    [vec2ToJson](Pillar::Entity e) -> json {
                        if (!e.HasComponent<MovingPlatformComponent>()) return nullptr;
                        auto& c = e.GetComponent<MovingPlatformComponent>();
                        return json{
                            {"start", vec2ToJson(c.Start)},
                            {"end", vec2ToJson(c.End)},
                            {"halfExtents", vec2ToJson(c.HalfExtents)},
                            {"speed", c.Speed},
                            {"pauseTime", c.PauseTime}
                        };
                    },
                    [jsonToVec2](Pillar::Entity e, const json& j) {
                        auto& c = e.HasComponent<MovingPlatformComponent>() ? e.GetComponent<MovingPlatformComponent>() : e.AddComponent<MovingPlatformComponent>();
                        if (j.contains("start") && j["start"].is_array())
                            c.Start = jsonToVec2(j["start"]);
                        if (j.contains("end") && j["end"].is_array())
                            c.End = jsonToVec2(j["end"]);
                        if (j.contains("halfExtents") && j["halfExtents"].is_array())
                            c.HalfExtents = jsonToVec2(j["halfExtents"]);
                        c.Speed = j.value("speed", 2.0f);
                        c.PauseTime = j.value("pauseTime", 0.4f);
                        c.PauseTimer = 0.0f;
                        c.Forward = true;
                    },
                    [](Pillar::Entity src, Pillar::Entity dst) {
                        if (!src.HasComponent<MovingPlatformComponent>()) return;
                        auto& s = src.GetComponent<MovingPlatformComponent>();
                        auto& d = dst.AddComponent<MovingPlatformComponent>();
                        d = s;
                        d.PauseTimer = 0.0f;
                        d.Forward = true;
                    });
            }
        }

        // ============================================
        // Entity Creation
        // ============================================
        void CreateBall(const glm::vec2& position)
        {
            m_BallEntity = m_Scene->CreateEntity("GolfBall");

            auto& transform = m_BallEntity.GetComponent<Pillar::TransformComponent>();
            transform.Position = glm::vec3(position, 0.0f);
            transform.Scale = glm::vec3(GolfBallComponent::Radius * 2.0f);

            // Add rigidbody
            m_BallEntity.AddComponent<Pillar::RigidbodyComponent>(b2_dynamicBody);

            // Add collider with bouncy physics
            auto collider = Pillar::ColliderComponent::Circle(GolfBallComponent::Radius);
            collider.Restitution = GolfBallComponent::Restitution;
            collider.Friction = GolfBallComponent::Friction;
            collider.Density = 1.0f;
            m_BallEntity.AddComponent<Pillar::ColliderComponent>(collider);

            // Add game component
            auto& ball = m_BallEntity.AddComponent<GolfBallComponent>();
            ball.LastShotPosition = position;
        }

        void CreateGoal(const glm::vec2& position, int par)
        {
            m_GoalEntity = m_Scene->CreateEntity("Goal");

            auto& transform = m_GoalEntity.GetComponent<Pillar::TransformComponent>();
            transform.Position = glm::vec3(position, 0.0f);

            auto& goal = m_GoalEntity.AddComponent<GoalComponent>();
            goal.ParScore = par;

            // Ensure goal has a circular sensor collider for overlap checks
            m_GoalEntity.AddComponent<Pillar::RigidbodyComponent>(b2_staticBody);
            auto sensor = Pillar::ColliderComponent::Circle(GoalComponent::VisualRadius);
            sensor.IsSensor = true;
            m_GoalEntity.AddComponent<Pillar::ColliderComponent>(sensor);
        }

        void CreateWalls(const std::vector<LevelData::WallDef>& wallDefs)
        {
            for (const auto& wallDef : wallDefs)
            {
                auto wallEntity = m_Scene->CreateEntity("Wall");

                auto& transform = wallEntity.GetComponent<Pillar::TransformComponent>();
                const bool isVertical = wallDef.HalfExtents.y > wallDef.HalfExtents.x;
                const float fullLength = (isVertical ? wallDef.HalfExtents.y : wallDef.HalfExtents.x) * 2.0f;
                const float fullThickness = (isVertical ? wallDef.HalfExtents.x : wallDef.HalfExtents.y) * 2.0f;

                const float usableLength = glm::max(0.1f, fullLength - WallEdgeInset * 2.0f);
                const float usableThickness = glm::max(0.05f, fullThickness);

                transform.Position = glm::vec3(wallDef.Position, 0.0f);
                transform.Rotation = wallDef.Rotation + (isVertical ? glm::half_pi<float>() : 0.0f);
                transform.Scale = glm::vec3(usableLength, usableThickness, 1.0f);

                // Static rigidbody
                wallEntity.AddComponent<Pillar::RigidbodyComponent>(b2_staticBody);

                // Box collider with some bounce
                glm::vec2 colliderHalfExtents = {usableLength * 0.5f, usableThickness * 0.5f};
                auto collider = Pillar::ColliderComponent::Box(colliderHalfExtents);
                collider.Restitution = 0.8f; // Walls are bouncy
                collider.Friction = 0.3f;
                wallEntity.AddComponent<Pillar::ColliderComponent>(collider);

                // Wall component for rendering
                wallEntity.AddComponent<WallComponent>();
            }
        }

        void CreateGravityWells(const std::vector<LevelData::GravityWellDef>& wellDefs)
        {
            m_GravityWells.clear();

            for (const auto& wellDef : wellDefs)
            {
                auto wellEntity = m_Scene->CreateEntity("GravityWell");

                auto& transform = wellEntity.GetComponent<Pillar::TransformComponent>();
                transform.Position = glm::vec3(wellDef.Position, 0.0f);
                transform.Scale = glm::vec3(wellDef.Radius * 2.0f);

                auto& well = wellEntity.AddComponent<GravityWellComponent>();
                well.Radius = wellDef.Radius;
                well.Strength = wellDef.Strength;
                well.IsRepulsor = wellDef.IsRepulsor;

                m_GravityWells.push_back(wellEntity);
            }
        }

        void CreateBoostPads(const std::vector<LevelData::BoostPadDef>& padDefs)
        {
            m_BoostPads.clear();

            for (const auto& padDef : padDefs)
            {
                auto padEntity = m_Scene->CreateEntity("BoostPad");

                auto& transform = padEntity.GetComponent<Pillar::TransformComponent>();
                transform.Position = glm::vec3(padDef.Position, 0.0f);
                transform.Scale = glm::vec3(padDef.Size, 1.0f);

                auto& pad = padEntity.AddComponent<BoostPadComponent>();
                pad.Size = padDef.Size;
                pad.Direction = glm::length(padDef.Direction) > 0.001f ? glm::normalize(padDef.Direction) : glm::vec2(1.0f, 0.0f);
                pad.Strength = padDef.Strength;
                pad.Cooldown = 0.0f;

                m_BoostPads.push_back({padEntity});
            }
        }

        void CreateMovingPlatforms(const std::vector<LevelData::MovingPlatformDef>& platformDefs)
        {
            m_MovingPlatforms.clear();

            for (const auto& platformDef : platformDefs)
            {
                auto platformEntity = m_Scene->CreateEntity("MovingPlatform");

                auto& transform = platformEntity.GetComponent<Pillar::TransformComponent>();
                transform.Position = glm::vec3(platformDef.Start, 0.0f);
                transform.Scale = glm::vec3(platformDef.HalfExtents * 2.0f, 1.0f);

                // Kinematic body so it can shove the ball without being pushed back
                platformEntity.AddComponent<Pillar::RigidbodyComponent>(b2_kinematicBody);

                auto collider = Pillar::ColliderComponent::Box(platformDef.HalfExtents);
                collider.Restitution = 0.85f;
                collider.Friction = 0.25f;
                platformEntity.AddComponent<Pillar::ColliderComponent>(collider);

                auto& wall = platformEntity.AddComponent<WallComponent>();
                wall.Color = {0.45f, 0.72f, 0.95f, 1.0f};

                auto& platform = platformEntity.AddComponent<MovingPlatformComponent>();
                platform.Start = platformDef.Start;
                platform.End = platformDef.End;
                platform.HalfExtents = platformDef.HalfExtents;
                platform.Speed = platformDef.Speed;
                platform.PauseTime = platformDef.PauseTime;
                platform.PauseTimer = 0.0f;
                platform.Forward = true;

                m_MovingPlatforms.push_back({platformEntity});
            }
        }

        // ============================================
        // Game State Update
        // ============================================
        void ApplyGravityWells(float /*dt*/)
        {
            if (!m_BallEntity || !m_BallEntity.HasComponent<Pillar::RigidbodyComponent>())
                return;

            auto& rb = m_BallEntity.GetComponent<Pillar::RigidbodyComponent>();
            auto& transform = m_BallEntity.GetComponent<Pillar::TransformComponent>();
            if (!rb.Body)
                return;

            glm::vec2 ballPos(transform.Position.x, transform.Position.y);

            for (auto& wellEntity : m_GravityWells)
            {
                if (!wellEntity)
                    continue;

                auto& wellTransform = wellEntity.GetComponent<Pillar::TransformComponent>();
                auto& well = wellEntity.GetComponent<GravityWellComponent>();

                glm::vec2 toWell = glm::vec2(wellTransform.Position.x, wellTransform.Position.y) - ballPos;
                float distance = glm::length(toWell);
                if (distance < 0.001f || distance > well.Radius)
                    continue;

                glm::vec2 direction = toWell / distance;

                // Smooth falloff so force peaks near center and fades at the edge
                float falloff = 1.0f - (distance / well.Radius);
                float forceMagnitude = well.Strength * falloff * falloff;
                if (well.IsRepulsor)
                    forceMagnitude *= -1.0f;

                b2Vec2 force(direction.x * forceMagnitude, direction.y * forceMagnitude);
                rb.Body->ApplyForceToCenter(force, true);
            }
        }

        void ApplyBoostPads(float dt)
        {
            if (!m_BallEntity || !m_BallEntity.HasComponent<Pillar::RigidbodyComponent>())
                return;

            auto& rb = m_BallEntity.GetComponent<Pillar::RigidbodyComponent>();
            auto& transform = m_BallEntity.GetComponent<Pillar::TransformComponent>();
            if (!rb.Body)
                return;

            glm::vec2 ballPos(transform.Position.x, transform.Position.y);

            for (auto& runtime : m_BoostPads)
            {
                auto& pad = runtime.Entity.GetComponent<BoostPadComponent>();
                auto& padTransform = runtime.Entity.GetComponent<Pillar::TransformComponent>();

                pad.Cooldown = glm::max(0.0f, pad.Cooldown - dt);

                glm::vec2 halfSize = pad.Size * 0.5f;
                glm::vec2 min = {padTransform.Position.x - halfSize.x, padTransform.Position.y - halfSize.y};
                glm::vec2 max = {padTransform.Position.x + halfSize.x, padTransform.Position.y + halfSize.y};

                bool inside = ballPos.x >= min.x && ballPos.x <= max.x && ballPos.y >= min.y && ballPos.y <= max.y;
                if (!inside || pad.Cooldown > 0.0f)
                    continue;

                glm::vec2 dir = glm::length(pad.Direction) > 0.001f ? glm::normalize(pad.Direction) : glm::vec2(1.0f, 0.0f);
                b2Vec2 impulse(dir.x * pad.Strength, dir.y * pad.Strength);
                rb.Body->ApplyLinearImpulseToCenter(impulse, true);

                pad.Cooldown = 0.4f;
                m_Audio.PlayBoost();
            }
        }

        void UpdateGameState(float dt)
        {
            if (!m_BallEntity || !m_GoalEntity)
                return;

            auto& ball = m_BallEntity.GetComponent<GolfBallComponent>();
            auto& ballTransform = m_BallEntity.GetComponent<Pillar::TransformComponent>();
            auto& goal = m_GoalEntity.GetComponent<GoalComponent>();
            auto& goalTransform = m_GoalEntity.GetComponent<Pillar::TransformComponent>();

            // Get ball velocity from Box2D
            glm::vec2 ballVelocity(0.0f);
            if (m_BallEntity.HasComponent<Pillar::RigidbodyComponent>())
            {
                auto& rb = m_BallEntity.GetComponent<Pillar::RigidbodyComponent>();
                if (rb.Body)
                {
                    b2Vec2 vel = rb.Body->GetLinearVelocity();
                    ballVelocity = {vel.x, vel.y};

                    // Apply linear damping manually for smoother stopping
                    float speed = glm::length(ballVelocity);
                    if (speed > 0.01f)
                    {
                        float dampingFactor = std::exp(-GolfBallComponent::LinearDamping * dt);
                        rb.Body->SetLinearVelocity(b2Vec2(ballVelocity.x * dampingFactor, 
                                                           ballVelocity.y * dampingFactor));
                    }
                }
            }

            // Apply gravity well forces for next physics tick
            ApplyGravityWells(dt);
            ApplyBoostPads(dt);

            glm::vec2 ballPos(ballTransform.Position.x, ballTransform.Position.y);
            glm::vec2 goalPos(goalTransform.Position.x, goalTransform.Position.y);

            float ballSpeed = glm::length(ballVelocity);
            ball.IsMoving = ballSpeed > 0.15f;

            EmitTrailParticles(ballPos, ballVelocity, dt);
            EmitWellAura(dt);

            // Check if ball is in goal area
            float distanceToGoal = glm::length(ballPos - goalPos);
            float captureRadius = GoalComponent::VisualRadius * 0.5f; // tighter capture per request

            if (m_GameState == GameState::BallMoving)
            {
                // Check for goal capture (center within half hole radius)
                if (distanceToGoal <= captureRadius)
                {
                    goal.Captured = true;
                    ball.InGoal = true;
                    m_BallHidden = true;

                    EmitGoalBurst(ballPos);

                    // Disable physics so it stays put
                    if (m_BallEntity.HasComponent<Pillar::RigidbodyComponent>())
                    {
                        auto& rb = m_BallEntity.GetComponent<Pillar::RigidbodyComponent>();
                        if (rb.Body) rb.Body->SetEnabled(false);
                    }

                    m_GameState = GameState::LevelComplete;
                    m_ShowLevelComplete = true;
                    m_LevelCompleteTimer = 0.0f;
                    m_BestShots[m_CurrentLevelIndex] = std::min(m_BestShots[m_CurrentLevelIndex], m_ShotCount);
                    m_Audio.PlayGoal();
                    PIL_INFO("Level Complete! Shots: {}, Par: {}", m_ShotCount, goal.ParScore);
                }
                // Ball stopped moving - back to aiming
                else if (!ball.IsMoving)
                {
                    m_GameState = GameState::Aiming;
                }
            }

            // Level complete UI timer
            if (m_ShowLevelComplete)
            {
                m_LevelCompleteTimer += dt;
            }
        }

        void UpdateMovingPlatforms(float dt)
        {
            if (m_MovingPlatforms.empty())
                return;

            for (auto& runtime : m_MovingPlatforms)
            {
                auto& transform = runtime.Entity.GetComponent<Pillar::TransformComponent>();
                auto& platform = runtime.Entity.GetComponent<MovingPlatformComponent>();

                // Pause when at endpoints
                if (platform.PauseTimer > 0.0f)
                {
                    platform.PauseTimer = glm::max(0.0f, platform.PauseTimer - dt);
                    if (runtime.Entity.HasComponent<Pillar::RigidbodyComponent>())
                    {
                        auto& rb = runtime.Entity.GetComponent<Pillar::RigidbodyComponent>();
                        if (rb.Body)
                            rb.Body->SetLinearVelocity(b2Vec2(0.0f, 0.0f));
                    }
                    continue;
                }

                glm::vec2 target = platform.Forward ? platform.End : platform.Start;
                glm::vec2 current(transform.Position.x, transform.Position.y);
                glm::vec2 toTarget = target - current;
                float distance = glm::length(toTarget);

                if (distance < 0.01f)
                {
                    platform.Forward = !platform.Forward;
                    platform.PauseTimer = platform.PauseTime;
                    continue;
                }

                glm::vec2 dir = toTarget / glm::max(distance, 0.0001f);
                float moveDistance = platform.Speed * dt;
                glm::vec2 delta = dir * glm::min(moveDistance, distance);

                transform.Position.x += delta.x;
                transform.Position.y += delta.y;

                if (runtime.Entity.HasComponent<Pillar::RigidbodyComponent>())
                {
                    auto& rb = runtime.Entity.GetComponent<Pillar::RigidbodyComponent>();
                    if (rb.Body)
                    {
                        rb.Body->SetTransform(b2Vec2(transform.Position.x, transform.Position.y), transform.Rotation);
                        rb.Body->SetLinearVelocity(b2Vec2(delta.x / dt, delta.y / dt));
                    }
                }
            }
        }

        // ============================================
        // Input Handling
        // ============================================
        bool OnMouseButtonPressed(Pillar::MouseButtonPressedEvent& e)
        {
            if (e.GetMouseButton() != PIL_MOUSE_BUTTON_LEFT)
                return false;

            // Ignore clicks in main menu (handled by ImGui)
            if (m_GameState == GameState::MainMenu)
                return false;

            // Handle level complete screen
            if (m_ShowLevelComplete && m_LevelCompleteTimer > 0.5f)
            {
                NextLevel();
                return true;
            }

            // Only shoot when aiming
            if (m_GameState != GameState::Aiming)
                return false;

            ShootBall();
            return true;
        }

        void ShootBall()
        {
            if (!m_BallEntity)
                return;

            auto& ball = m_BallEntity.GetComponent<GolfBallComponent>();
            auto& transform = m_BallEntity.GetComponent<Pillar::TransformComponent>();

            if (!m_BallEntity.HasComponent<Pillar::RigidbodyComponent>())
                return;

            auto& rb = m_BallEntity.GetComponent<Pillar::RigidbodyComponent>();
            if (!rb.Body)
                return;

            // Get mouse position in world coordinates
            auto [mouseX, mouseY] = Pillar::Input::GetMousePosition();
            auto& camera = m_CameraController.GetCamera();

            // Convert screen to world coordinates
            glm::vec2 windowSize(
                Pillar::Application::Get().GetWindow().GetWidth(),
                Pillar::Application::Get().GetWindow().GetHeight()
            );

            // Normalize mouse position (0-1)
            glm::vec2 normalizedMouse(
                mouseX / windowSize.x,
                1.0f - (mouseY / windowSize.y) // Flip Y
            );

            // Get camera bounds
            float zoomLevel = m_CameraController.GetZoomLevel();
            float aspectRatio = windowSize.x / windowSize.y;
            float halfWidth = aspectRatio * zoomLevel;
            float halfHeight = zoomLevel;

            glm::vec3 camPos = camera.GetPosition();
            glm::vec2 worldMouse(
                camPos.x + (normalizedMouse.x * 2.0f - 1.0f) * halfWidth,
                camPos.y + (normalizedMouse.y * 2.0f - 1.0f) * halfHeight
            );

            // Calculate direction and power
            glm::vec2 ballPos(transform.Position.x, transform.Position.y);
            glm::vec2 direction = worldMouse - ballPos;
            float distance = glm::length(direction);

            if (distance < 0.1f)
                return; // Too close to ball

            direction = glm::normalize(direction);

            // Calculate power based on distance (clamped)
            float power = glm::clamp(distance * 2.0f, ball.MinPower, ball.MaxPower);

            // Apply impulse
            glm::vec2 impulse = direction * power;
            rb.Body->ApplyLinearImpulseToCenter(b2Vec2(impulse.x, impulse.y), true);

            // Update state
            ball.ShotCount++;
            ball.LastShotPosition = ballPos;
            m_ShotCount++;
            m_GameState = GameState::BallMoving;

            m_Audio.PlayShoot();
        }

        // ============================================
        // Rendering
        // ============================================
        void Render()
        {
            // Disable depth for this 2D pass so alpha textures (boosters) don't occlude later draws
            const GLboolean wasDepthEnabled = glIsEnabled(GL_DEPTH_TEST);
            GLboolean wasDepthMask = GL_TRUE;
            glGetBooleanv(GL_DEPTH_WRITEMASK, &wasDepthMask);
            glDisable(GL_DEPTH_TEST);
            glDepthMask(GL_FALSE);

            // Clear screen with a nice dark green (golf course feel)
            Pillar::Renderer::SetClearColor({0.05f, 0.12f, 0.08f, 1.0f});
            Pillar::Renderer::Clear();

            Pillar::Renderer2DBackend::ResetStats();
            Pillar::Renderer2DBackend::BeginScene(m_CameraController.GetCamera());

            // Render in order: background -> goal -> walls -> ball -> aim line
            // (lower z = rendered first/behind)
            DrawBackground();
            DrawGravityWells();
            DrawBoostPads();
            DrawGoal();
            DrawWalls();
            DrawParticles();
            DrawBall();

            // Draw aim line when aiming (on top of everything)
            if (m_GameState == GameState::Aiming && m_BallEntity)
            {
                DrawAimLine();
            }

            Pillar::Renderer2DBackend::EndScene();

            // Restore depth state
            glDepthMask(wasDepthMask);
            if (wasDepthEnabled)
                glEnable(GL_DEPTH_TEST);
            else
                glDisable(GL_DEPTH_TEST);
        }

        void DrawBackground()
        {
            // Cover a generous area around the level with a built-in checker tint (no extra texture).
            const float width = 40.0f;
            const float height = 24.0f;
            const glm::vec3 pos = {0.0f, 0.0f, ZLayer::Background};

            const float tileSize = 2.0f; // matches current UV scale (uvMax = size * 0.5)
            const float startX = -width * 0.5f;
            const float startY = -height * 0.5f;
            const float uvTile = tileSize * 0.5f; // keeps existing texel density

            if (m_TextureGrass)
            {
                for (float y = startY, j = 0.0f; y < startY + height; y += tileSize, j += 1.0f)
                {
                    for (float x = startX, i = 0.0f; x < startX + width; x += tileSize, i += 1.0f)
                    {
                        const bool dark = (static_cast<int>(i) + static_cast<int>(j)) % 2 != 0;
                        const float tint = dark ? 0.9f : 1.0f;

                        Pillar::Renderer2DBackend::DrawQuad(
                            glm::vec3{x + tileSize * 0.5f, y + tileSize * 0.5f, pos.z},
                            glm::vec2{tileSize, tileSize},
                            glm::vec4(tint, tint, tint, 1.0f),
                            m_TextureGrass,
                            glm::vec2{0.0f, 0.0f},
                            glm::vec2{uvTile, uvTile},
                            false,
                            false
                        );
                    }
                }
            }
            else
            {
                for (float y = startY, j = 0.0f; y < startY + height; y += tileSize, j += 1.0f)
                {
                    for (float x = startX, i = 0.0f; x < startX + width; x += tileSize, i += 1.0f)
                    {
                        const bool dark = (static_cast<int>(i) + static_cast<int>(j)) % 2 != 0;
                        const glm::vec4 color = dark
                            ? glm::vec4(0.16f, 0.40f, 0.18f, 1.0f)
                            : glm::vec4(0.20f, 0.50f, 0.22f, 1.0f);

                        Pillar::Renderer2DBackend::DrawQuad(
                            glm::vec3{x + tileSize * 0.5f, y + tileSize * 0.5f, pos.z},
                            glm::vec2{tileSize, tileSize},
                            color
                        );
                    }
                }
            }
        }

        void DrawGravityWells()
        {
            if (m_GravityWells.empty())
                return;

            const float pulse = 0.12f * std::sin(m_Time * 2.2f);

            for (auto& wellEntity : m_GravityWells)
            {
                auto& transform = wellEntity.GetComponent<Pillar::TransformComponent>();
                auto& well = wellEntity.GetComponent<GravityWellComponent>();

                glm::vec2 pos(transform.Position.x, transform.Position.y);
                float radius = well.Radius;

                glm::vec4 baseColor = well.IsRepulsor
                    ? glm::vec4(1.0f, 0.35f, 0.8f, 0.6f)
                    : glm::vec4(0.3f, 0.9f, 1.0f, 0.6f);

                glm::vec4 ringColor = baseColor;
                ringColor.a = 0.35f + pulse;

                glm::vec4 coreColor = baseColor;
                coreColor.a = 0.9f;

                glm::vec2 outerSize = {radius * 2.0f, radius * 2.0f};
                glm::vec2 innerSize = outerSize * 0.6f;
            }
        }

        void DrawBoostPads()
        {
            if (m_BoostPads.empty())
                return;

            for (auto& runtime : m_BoostPads)
            {
                auto& transform = runtime.Entity.GetComponent<Pillar::TransformComponent>();
                auto& pad = runtime.Entity.GetComponent<BoostPadComponent>();

                glm::vec2 pos(transform.Position.x, transform.Position.y);
                glm::vec2 size = pad.Size;
                float rotation = std::atan2(pad.Direction.y, pad.Direction.x);

                // Preserve the new texture's 16:9 aspect while fitting the intended pad footprint
                const float targetAspect = 16.0f / 9.0f;
                const float currentAspect = size.x / glm::max(size.y, 0.001f);
                if (currentAspect > targetAspect)
                    size.y = size.x / targetAspect;
                else
                    size.x = size.y * targetAspect;

                const glm::vec4 baseTint(1.0f, 1.0f, 1.0f, 0.95f);
                const glm::vec4 arrowTint(0.95f, 0.95f, 0.35f, 0.7f);

                if (m_TextureBooster)
                {
                    Pillar::Renderer2DBackend::DrawRotatedQuad(
                        {pos.x, pos.y},
                        size,
                        rotation,
                        baseTint,
                        m_TextureBooster
                    );

                }
                else
                {
                    glm::vec4 baseColor(0.22f, 0.82f, 0.95f, 0.7f);
                    glm::vec4 arrowColor(0.95f, 0.95f, 0.35f, 0.9f);

                    Pillar::Renderer2DBackend::DrawRotatedQuad(
                        {pos.x, pos.y},
                        size,
                        rotation,
                        baseColor
                    );

                    Pillar::Renderer2DBackend::DrawRotatedQuad(
                        {pos.x + pad.Direction.x * 0.1f, pos.y + pad.Direction.y * 0.1f},
                        size * 0.5f,
                        rotation,
                        arrowColor
                    );
                }
            }
        }

        void DrawBall()
        {
            if (!m_BallEntity)
                return;
            if (m_BallHidden)
                return;

            auto& transform = m_BallEntity.GetComponent<Pillar::TransformComponent>();
            glm::vec2 pos(transform.Position.x, transform.Position.y);
            float ballSize = GolfBallComponent::Radius * 2.0f;

            // Ball with texture (use 3D position overload for proper z-ordering)
            if (m_TextureBall)
            {
                Pillar::Renderer2DBackend::DrawQuad(
                    {pos.x, pos.y, ZLayer::Ball},
                    {ballSize, ballSize},
                    glm::vec4(1.0f),  // White tint = show texture as-is
                    m_TextureBall,
                    {0.0f, 0.0f},
                    {1.0f, 1.0f},
                    false,
                    false
                );
            }
            else
            {
                // Fallback: white circle (colored quad)
                Pillar::Renderer2DBackend::DrawQuad(
                    {pos.x, pos.y},
                    {ballSize, ballSize},
                    {1.0f, 1.0f, 1.0f, 1.0f}
                );
            }
        }

        void DrawGoal()
        {
            if (!m_GoalEntity)
                return;

            auto& transform = m_GoalEntity.GetComponent<Pillar::TransformComponent>();
            auto& goal = m_GoalEntity.GetComponent<GoalComponent>();
            glm::vec2 pos(transform.Position.x, transform.Position.y);
            float goalSize = GoalComponent::VisualRadius * 2.4f;

            // Textured goal (hole); tint green if captured
            glm::vec4 tint = goal.Captured ? glm::vec4(0.7f, 1.0f, 0.7f, 1.0f) : glm::vec4(1.0f);

            if (m_TextureGoal)
            {
                Pillar::Renderer2DBackend::DrawQuad(
                    {pos.x, pos.y, ZLayer::Goal},
                    {goalSize, goalSize},
                    tint,
                    m_TextureGoal,
                    {0.0f, 0.0f},
                    {1.0f, 1.0f},
                    false,
                    false
                );
            }
            else
            {
                // Fallback: dark circle
                Pillar::Renderer2DBackend::DrawQuad(
                    {pos.x, pos.y},
                    {goalSize, goalSize},
                    {0.1f, 0.1f, 0.1f, 1.0f}
                );
            }
        }

        void DrawWalls()
        {
            auto view = m_Scene->GetRegistry().view<Pillar::TransformComponent, WallComponent>();

            for (auto entity : view)
            {
                auto& transform = view.get<Pillar::TransformComponent>(entity);
                auto& wall = view.get<WallComponent>(entity);

                if (!wall.IsVisible)
                    continue;

                glm::vec2 pos(transform.Position.x, transform.Position.y);
                glm::vec2 size(transform.Scale.x, transform.Scale.y);

                const float wallLength = size.x;
                const float wallThickness = size.y;

                const float usableLength = glm::max(0.0f, wallLength - WallEdgeInset * 2.0f);

                // Decide how many planks we need; allow slight overlap to remove visible gaps
                const int segmentCount = glm::max(1, static_cast<int>(std::ceil(usableLength / DominoSize.x)));
                const float step = segmentCount > 1
                    ? (usableLength - DominoSize.x) / static_cast<float>(segmentCount - 1)
                    : 0.0f; // center-to-center spacing; <= DominoSize.x creates overlap instead of gaps

                const float renderRotation = transform.Rotation;
                const glm::vec2 forward = {std::cos(renderRotation), std::sin(renderRotation)};
                const glm::vec2 start = pos - forward * (usableLength * 0.5f);

                const glm::vec2 segmentSize = {DominoSize.x, glm::min(DominoSize.y, wallThickness)};

                for (int i = 0; i < segmentCount; ++i)
                {
                    const float offset = (DominoSize.x * 0.5f) + i * step;
                    const glm::vec2 segmentCenter = start + forward * offset;

                    // Shadow pass (constant world offset so it reads as a shadow, not a skew)

                    // Color/texture pass
                    if (m_TextureWall)
                    {
                        Pillar::Renderer2DBackend::DrawRotatedQuad(
                            {segmentCenter.x, segmentCenter.y},
                            segmentSize,
                            renderRotation,
                            glm::vec4(1.0f),
                            m_TextureWall
                        );
                    }
                    else
                    {
                        Pillar::Renderer2DBackend::DrawRotatedQuad(
                            {segmentCenter.x, segmentCenter.y},
                            segmentSize,
                            renderRotation,
                            wall.Color
                        );
                    }
                }
            }
        }

        void DrawParticles()
        {
            for (const auto& particle : m_Particles)
            {
                if (particle.Life <= 0.0f)
                    continue;

                float t = particle.Life / particle.MaxLife;
                glm::vec4 color = glm::mix(particle.ColorEnd, particle.ColorStart, t);
                float size = glm::mix(particle.SizeEnd, particle.SizeStart, t);

                Pillar::Renderer2DBackend::DrawQuad(
                    glm::vec2{particle.Position.x, particle.Position.y},
                    {size, size},
                    color
                );
            }
        }

        void DrawAimLine()
        {
            auto& transform = m_BallEntity.GetComponent<Pillar::TransformComponent>();
            auto& ball = m_BallEntity.GetComponent<GolfBallComponent>();

            glm::vec2 ballPos(transform.Position.x, transform.Position.y);

            // Get mouse world position
            auto [mouseX, mouseY] = Pillar::Input::GetMousePosition();
            auto& camera = m_CameraController.GetCamera();

            glm::vec2 windowSize(
                Pillar::Application::Get().GetWindow().GetWidth(),
                Pillar::Application::Get().GetWindow().GetHeight()
            );

            glm::vec2 normalizedMouse(
                mouseX / windowSize.x,
                1.0f - (mouseY / windowSize.y)
            );

            float zoomLevel = m_CameraController.GetZoomLevel();
            float aspectRatio = windowSize.x / windowSize.y;
            float halfWidth = aspectRatio * zoomLevel;
            float halfHeight = zoomLevel;

            glm::vec3 camPos = camera.GetPosition();
            glm::vec2 worldMouse(
                camPos.x + (normalizedMouse.x * 2.0f - 1.0f) * halfWidth,
                camPos.y + (normalizedMouse.y * 2.0f - 1.0f) * halfHeight
            );

            glm::vec2 direction = worldMouse - ballPos;
            float distance = glm::length(direction);

            if (distance < 0.1f)
                return;

            direction = glm::normalize(direction);
            float power = glm::clamp(distance * 2.0f, ball.MinPower, ball.MaxPower);
            float powerRatio = (power - ball.MinPower) / (ball.MaxPower - ball.MinPower);

            // Color from green (low power) to red (high power)
            glm::vec4 lineColor(
                powerRatio,
                1.0f - powerRatio * 0.7f,
                0.2f,
                0.8f
            );

            // Draw dotted aim line
            float lineLength = glm::min(distance, 3.0f);
            int numDots = static_cast<int>(lineLength * 5);

            for (int i = 1; i <= numDots; ++i)
            {
                float t = static_cast<float>(i) / static_cast<float>(numDots + 1);
                glm::vec2 dotPos = ballPos + direction * (t * lineLength);

                // Fade alpha along the line
                float dotAlpha = lineColor.a * (1.0f - t * 0.5f);

                Pillar::Renderer2DBackend::DrawQuad(
                    {dotPos.x, dotPos.y, ZLayer::AimLine},
                    {0.08f, 0.08f},
                    {lineColor.r, lineColor.g, lineColor.b, dotAlpha},
                    nullptr,
                    {0.0f, 0.0f},
                    {1.0f, 1.0f},
                    false,
                    false
                );
            }

            // Power indicator bar
            float barWidth = 0.15f;
            float barHeight = 1.5f;
            glm::vec2 barPos = ballPos + glm::vec2(-1.5f, 0.0f);

            // Background
            Pillar::Renderer2DBackend::DrawQuad(
                {barPos.x, barPos.y, ZLayer::AimLine},
                {barWidth, barHeight},
                {0.2f, 0.2f, 0.2f, 0.7f},
                nullptr,
                {0.0f, 0.0f},
                {1.0f, 1.0f},
                false,
                false
            );

            // Fill
            float fillHeight = barHeight * powerRatio;
            Pillar::Renderer2DBackend::DrawQuad(
                {barPos.x, barPos.y - barHeight * 0.5f + fillHeight * 0.5f, ZLayer::AimLine + 0.01f},
                {barWidth * 0.8f, fillHeight},
                lineColor,
                nullptr,
                {0.0f, 0.0f},
                {1.0f, 1.0f},
                false,
                false
            );
        }

        // ============================================
        // UI Styling
        // ============================================
        void EnsureUIStyle()
        {
            if (m_StyleInitialized)
                return;

            ImGuiStyle& style = ImGui::GetStyle();
            style.WindowPadding = ImVec2(18.0f, 14.0f);
            style.FramePadding = ImVec2(12.0f, 8.0f);
            style.ItemSpacing = ImVec2(12.0f, 10.0f);
            style.WindowRounding = 14.0f;
            style.ChildRounding = 12.0f;
            style.FrameRounding = 12.0f;
            style.PopupRounding = 12.0f;
            style.GrabRounding = 10.0f;
            style.TabRounding = 9.0f;
            style.WindowBorderSize = 0.0f;
            style.FrameBorderSize = 0.0f;
            style.ScrollbarSize = 12.0f;

            const ImVec4 accent(0.24f, 0.64f, 0.94f, 1.0f);
            const ImVec4 accentHover(0.30f, 0.70f, 1.00f, 1.0f);
            const ImVec4 accentActive(0.18f, 0.58f, 0.92f, 1.0f);
            const ImVec4 bg(0.07f, 0.08f, 0.11f, 0.96f);
            const ImVec4 bgAlt(0.09f, 0.10f, 0.13f, 0.95f);
            const ImVec4 surface(0.12f, 0.14f, 0.18f, 1.0f);

            style.Colors[ImGuiCol_Text] = ImVec4(0.90f, 0.93f, 0.98f, 1.0f);
            style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.55f, 0.60f, 0.70f, 1.0f);
            style.Colors[ImGuiCol_WindowBg] = bg;
            style.Colors[ImGuiCol_ChildBg] = ImVec4(0.08f, 0.09f, 0.13f, 0.92f);
            style.Colors[ImGuiCol_PopupBg] = bgAlt;
            style.Colors[ImGuiCol_Border] = ImVec4(0.20f, 0.26f, 0.32f, 0.45f);
            style.Colors[ImGuiCol_FrameBg] = surface;
            style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.16f, 0.20f, 0.26f, 1.0f);
            style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.19f, 0.24f, 0.30f, 1.0f);
            style.Colors[ImGuiCol_TitleBg] = bg;
            style.Colors[ImGuiCol_TitleBgCollapsed] = bg;
            style.Colors[ImGuiCol_TitleBgActive] = bgAlt;
            style.Colors[ImGuiCol_MenuBarBg] = bgAlt;
            style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.05f, 0.05f, 0.07f, 0.60f);
            style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.18f, 0.22f, 0.28f, 0.80f);
            style.Colors[ImGuiCol_ScrollbarGrabHovered] = accentHover;
            style.Colors[ImGuiCol_ScrollbarGrabActive] = accentActive;
            style.Colors[ImGuiCol_CheckMark] = accent;
            style.Colors[ImGuiCol_SliderGrab] = accent;
            style.Colors[ImGuiCol_SliderGrabActive] = accentActive;
            style.Colors[ImGuiCol_Button] = ImVec4(0.16f, 0.19f, 0.24f, 1.0f);
            style.Colors[ImGuiCol_ButtonHovered] = accentHover;
            style.Colors[ImGuiCol_ButtonActive] = accentActive;
            style.Colors[ImGuiCol_Header] = ImVec4(0.14f, 0.18f, 0.23f, 1.0f);
            style.Colors[ImGuiCol_HeaderHovered] = accentHover;
            style.Colors[ImGuiCol_HeaderActive] = accentActive;
            style.Colors[ImGuiCol_Separator] = ImVec4(0.24f, 0.28f, 0.34f, 0.8f);
            style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.10f, 0.13f, 0.16f, 0.70f);
            style.Colors[ImGuiCol_ResizeGripHovered] = accentHover;
            style.Colors[ImGuiCol_ResizeGripActive] = accentActive;
            style.Colors[ImGuiCol_Tab] = ImVec4(0.14f, 0.17f, 0.22f, 1.0f);
            style.Colors[ImGuiCol_TabHovered] = accentHover;
            style.Colors[ImGuiCol_TabActive] = accentActive;
            style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.12f, 0.14f, 0.18f, 1.0f);
            style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.16f, 0.19f, 0.24f, 1.0f);
            style.Colors[ImGuiCol_PlotLines] = accent;
            style.Colors[ImGuiCol_PlotLinesHovered] = accentHover;
            style.Colors[ImGuiCol_PlotHistogram] = accent;
            style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.12f, 0.15f, 0.20f, 1.0f);
            style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.20f, 0.25f, 0.32f, 0.8f);
            style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.20f, 0.25f, 0.32f, 0.4f);
            style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.10f, 0.11f, 0.14f, 0.5f);
            style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.12f, 0.13f, 0.16f, 0.5f);
            style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.24f, 0.56f, 0.88f, 0.35f);
            style.Colors[ImGuiCol_DragDropTarget] = accent;
            style.Colors[ImGuiCol_NavHighlight] = accentHover;
            style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.80f, 0.80f, 0.80f, 0.30f);
            style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.04f, 0.04f, 0.04f, 0.55f);
            style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.04f, 0.05f, 0.06f, 0.70f);

            m_UiFont = LoadUIFont();
            ImGuiIO& io = ImGui::GetIO();
            if (m_UiFont)
                io.FontDefault = m_UiFont;

            m_StyleInitialized = true;
        }

        ImFont* LoadUIFont()
        {
            ImGuiIO& io = ImGui::GetIO();

            ImFontConfig cfg{};
            cfg.OversampleH = 3;
            cfg.OversampleV = 3;
            cfg.PixelSnapH = false;

            const float fontSize = 18.0f;
            const std::vector<std::string> fontCandidates = {
                Pillar::AssetManager::GetAssetPath("fonts/Inter-SemiBold.ttf"),
                Pillar::AssetManager::GetAssetPath("fonts/Inter-Medium.ttf"),
                Pillar::AssetManager::GetAssetPath("fonts/Manrope-SemiBold.ttf"),
                "C:/Windows/Fonts/segoeui.ttf",
                "C:/Windows/Fonts/SegoeUIVF.ttf",
                "C:/Windows/Fonts/verdana.ttf"
            };

            for (const auto& path : fontCandidates)
            {
                if (path.empty())
                    continue;

                if (ImFont* font = io.Fonts->AddFontFromFileTTF(path.c_str(), fontSize, &cfg))
                    return font;
            }

            return io.Fonts->AddFontDefault();
        }

        // ============================================
        // UI Rendering
        // ============================================
        void RenderUI()
        {
            // Render main menu if in that state
            if (m_GameState == GameState::MainMenu)
            {
                RenderMainMenu();
                return;
            }

            EnsureUIStyle();

            const LevelData& level = m_Levels[m_CurrentLevelIndex];
            auto& goal = m_GoalEntity.GetComponent<GoalComponent>();
            const char* levelName = level.Name ? level.Name : "Level";

            ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImVec2 origin = viewport->WorkPos;
            ImVec2 area = viewport->WorkSize;
            const ImVec4 accent(0.28f, 0.70f, 1.0f, 1.0f);
            const ImVec4 accentSoft(accent.x, accent.y, accent.z, 0.22f);
            const ImVec4 muted = ImVec4(0.68f, 0.76f, 0.88f, 1.0f);
            const ImVec4 panelBg = ImVec4(0.08f, 0.09f, 0.13f, 0.94f);
            const ImVec4 cardBg = ImVec4(0.11f, 0.13f, 0.17f, 0.96f);

            ImGui::PushFont(m_UiFont ? m_UiFont : ImGui::GetFont());
            ImGui::PushStyleColor(ImGuiCol_WindowBg, panelBg);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20.0f, 16.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(12.0f, 10.0f));

            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::SetNextWindowPos(ImVec2(origin.x + 18.0f, origin.y + 18.0f), ImGuiCond_Always);
            ImGui::SetNextWindowBgAlpha(0.94f);
            if (ImGui::Begin("##HUD", nullptr,
                ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_AlwaysAutoResize |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoSavedSettings))
            {
                ImGui::TextColored(accent, "Gravity Golf");
                ImGui::SameLine();
                ImGui::TextColored(muted, "Tutorial pack");

                if (m_TextureIcons)
                {
                    ImVec2 iconSize(30.0f, 30.0f);
                    ImGui::PushID("icon_row");
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10.0f, 0.0f));

                    DrawIconButton("restart", IconSlot::Restart, "Restart level", iconSize, ImVec4(1,1,1,1), [this]() { RestartLevel(); });
                    ImGui::SameLine();

                    const bool isMuted = m_IsMuted || m_Audio.GetMasterVolume() <= 0.001f;
                    ImVec4 muteTint = isMuted ? ImVec4(0.96f, 0.45f, 0.45f, 1.0f) : ImVec4(1,1,1,1);
                    DrawIconButton("mute", IconSlot::Mute, isMuted ? "Unmute" : "Mute", iconSize, muteTint, [this]() { ToggleMute(); });
                    ImGui::SameLine();

                    DrawIconButton("hand", IconSlot::Hand, m_ShowHelp ? "Hide help" : "Show help", iconSize,
                        m_ShowHelp ? ImVec4(0.70f, 0.90f, 1.0f, 1.0f) : ImVec4(1,1,1,1),
                        [this]() { m_ShowHelp = !m_ShowHelp; });

                    int stars = CalculateStars(m_ShotCount, goal.ParScore);
                    ImVec4 activeStar = ImVec4(0.95f, 0.85f, 0.30f, 1.0f);
                    ImVec4 inactiveStar = ImVec4(0.35f, 0.35f, 0.35f, 1.0f);
                    ImGui::SameLine();
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6, 0));
                    for (int i = 0; i < 3; ++i)
                    {
                        DrawIconButton(i == 0 ? "star0" : (i == 1 ? "star1" : "star2"), IconSlot::Star, "Par rating", iconSize,
                            i < stars ? activeStar : inactiveStar,
                            nullptr);
                        if (i < 2) ImGui::SameLine();
                    }
                    ImGui::PopStyleVar();

                    ImGui::PopStyleVar();
                    ImGui::PopID();
                }

                ImGui::Separator();

                ImGui::TextColored(ImVec4(0.93f, 0.86f, 0.62f, 1.0f), "Level %d", level.LevelNumber);
                ImGui::SameLine();
                ImGui::TextColored(muted, "%s", levelName);

                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Button, accentSoft);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(accent.x, accent.y, accent.z, 0.35f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(accent.x, accent.y, accent.z, 0.50f));
                if (ImGui::Button("Levels", ImVec2(70.0f, 0.0f)))
                    m_ShowLevelSelect = true;
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.15f, 0.15f, 0.8f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.20f, 0.20f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.45f, 0.25f, 0.25f, 1.0f));
                if (ImGui::Button("Menu", ImVec2(60.0f, 0.0f)))
                    ReturnToMainMenu();
                ImGui::PopStyleColor(3);
                ImGui::PopStyleColor(3);

                ImGui::Spacing();

                ImGui::PushStyleColor(ImGuiCol_ChildBg, cardBg);
                if (ImGui::BeginChild("##hud_card", ImVec2(0.0f, 132.0f), true, ImGuiWindowFlags_NoScrollbar))
                {
                    ImVec4 shotColor;
                    if (m_ShotCount <= goal.ParScore - 1)
                        shotColor = ImVec4(0.42f, 0.90f, 0.56f, 1.0f);
                    else if (m_ShotCount <= goal.ParScore)
                        shotColor = ImVec4(0.96f, 0.86f, 0.48f, 1.0f);
                    else
                        shotColor = ImVec4(0.96f, 0.46f, 0.36f, 1.0f);

                    int bestShots = (m_CurrentLevelIndex < static_cast<int>(m_BestShots.size())) ? m_BestShots[m_CurrentLevelIndex] : std::numeric_limits<int>::max();

                    ImGui::TextColored(shotColor, "Shots %d", m_ShotCount);
                    ImGui::SameLine();
                    ImGui::TextColored(muted, "(Par %d)", goal.ParScore);

                    float parWindow = glm::max(1.0f, static_cast<float>(goal.ParScore) + 3.0f);
                    float shotRatio = glm::clamp(static_cast<float>(m_ShotCount) / parWindow, 0.0f, 1.0f);
                    std::string shotLabel = std::to_string(m_ShotCount) + " / " + std::to_string(goal.ParScore) + " par";
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.14f, 0.16f, 0.20f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, shotColor);
                    ImGui::ProgressBar(shotRatio, ImVec2(280.0f, 18.0f), shotLabel.c_str());
                    ImGui::PopStyleColor(2);

                    if (bestShots != std::numeric_limits<int>::max())
                        ImGui::TextColored(muted, "Best: %d shots (%d stars)", bestShots, CalculateStars(bestShots, goal.ParScore));
                    else
                        ImGui::TextColored(muted, "Best: --");

                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.62f, 0.70f, 0.82f, 1.0f), "R to restart | Click to shoot");
                }
                ImGui::EndChild();
                ImGui::PopStyleColor();

                if (m_ShowHelp)
                {
                    ImGui::Separator();
                    ImGui::TextColored(accent, "Controls");
                    ImGui::Text("Left click: Shoot");
                    ImGui::Text("R: Restart level");
                    ImGui::Text("WASD: Move camera");
                    ImGui::Text("Mouse wheel: Zoom");
                }
            }

            ImGui::End();
            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor();
            ImGui::PopFont();

            if (m_ShowLevelComplete)
            {
                ImGui::SetNextWindowViewport(viewport->ID);
                ImVec2 center(origin.x + area.x * 0.5f, origin.y + area.y * 0.5f);
                ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
                ImGui::SetNextWindowBgAlpha(0.94f);
                ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.09f, 0.10f, 0.14f, 0.96f));
                ImGui::PushFont(m_UiFont ? m_UiFont : ImGui::GetFont());
                if (ImGui::Begin("##LevelComplete", nullptr,
                    ImGuiWindowFlags_NoTitleBar |
                    ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_AlwaysAutoResize |
                    ImGuiWindowFlags_NoMove |
                    ImGuiWindowFlags_NoSavedSettings))
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.94f, 0.86f, 0.40f, 1.0f));
                    ImGui::SetWindowFontScale(1.45f);
                    ImGui::Text("LEVEL COMPLETE");
                    ImGui::SetWindowFontScale(1.0f);
                    ImGui::PopStyleColor();

                    ImGui::Separator();
                    ImGui::Spacing();

                    int stars = CalculateStars(m_ShotCount, goal.ParScore);
                    ImVec4 starColor(0.95f, 0.85f, 0.30f, 1.0f);
                    ImVec4 emptyStarColor(0.35f, 0.40f, 0.48f, 1.0f);

                    ImGui::Text("Rating:");
                    ImGui::SameLine();
                    for (int i = 0; i < 3; ++i)
                    {
                        if (i < stars)
                            ImGui::TextColored(starColor, "*");
                        else
                            ImGui::TextColored(emptyStarColor, "*");
                        if (i < 2) ImGui::SameLine();
                    }

                    ImGui::Spacing();
                    ImGui::Text("Shots: %d  |  Par: %d", m_ShotCount, goal.ParScore);

                    if (m_ShotCount < goal.ParScore)
                        ImGui::TextColored(ImVec4(0.42f, 0.92f, 0.58f, 1.0f), "Under Par!");
                    else if (m_ShotCount == goal.ParScore)
                        ImGui::TextColored(ImVec4(0.96f, 0.86f, 0.48f, 1.0f), "Par!");
                    else
                        ImGui::TextColored(ImVec4(0.96f, 0.46f, 0.36f, 1.0f), "Over Par");

                    ImGui::Spacing();
                    ImGui::Separator();

                    if (m_LevelCompleteTimer > 0.5f)
                    {
                        if (m_CurrentLevelIndex < static_cast<int>(m_Levels.size()) - 1)
                            ImGui::TextColored(ImVec4(0.64f, 0.86f, 0.70f, 1.0f), "Click to continue...");
                        else
                            ImGui::TextColored(ImVec4(0.62f, 0.85f, 0.94f, 1.0f),
                                "Tutorial complete! Click to restart.");
                    }
                }
                ImGui::End();
                ImGui::PopFont();
                ImGui::PopStyleColor();
            }

            RenderLevelSelect();
        }

        void RenderLevelSelect()
        {
            if (!m_ShowLevelSelect)
                return;

            ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImVec2 origin = viewport->WorkPos;
            ImVec2 area = viewport->WorkSize;

            ImGui::SetNextWindowViewport(viewport->ID);
            ImVec2 center(origin.x + area.x * 0.5f, origin.y + area.y * 0.5f);
            ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowBgAlpha(0.93f);

            ImGui::PushFont(m_UiFont ? m_UiFont : ImGui::GetFont());
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.09f, 0.10f, 0.14f, 0.95f));

            if (ImGui::Begin("Level Select", &m_ShowLevelSelect,
                ImGuiWindowFlags_AlwaysAutoResize |
                ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoSavedSettings))
            {
                ImGui::TextColored(ImVec4(0.28f, 0.70f, 1.0f, 1.0f), "Jump to a level");
                ImGui::Separator();

                for (size_t i = 0; i < m_Levels.size(); ++i)
                {
                    const auto& level = m_Levels[i];
                    int best = (i < m_BestShots.size()) ? m_BestShots[i] : std::numeric_limits<int>::max();
                    int stars = best == std::numeric_limits<int>::max() ? -1 : CalculateStars(best, level.Par);

                    ImGui::PushID(static_cast<int>(i));

                    const char* levelName = level.Name ? level.Name : "Level";
                    ImGui::TextColored(ImVec4(0.90f, 0.92f, 0.98f, 1.0f), "%d. %s", level.LevelNumber, levelName);
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(0.68f, 0.76f, 0.88f, 1.0f), "Par %d", level.Par);

                    if (best == std::numeric_limits<int>::max())
                        ImGui::Text("Best: --");
                    else
                        ImGui::Text("Best: %d (stars: %d)", best, stars);

                    std::string buttonLabel = "Play##" + std::to_string(i);
                    if (ImGui::Button(buttonLabel.c_str(), ImVec2(90.0f, 0.0f)))
                    {
                        LoadLevel(static_cast<int>(i));
                        m_ShowLevelSelect = false;
                    }

                    ImGui::PopID();
                    ImGui::Separator();
                }
            }

            ImGui::End();
            ImGui::PopStyleColor();
            ImGui::PopFont();
        }

        // ============================================
        // Particle helpers (Phase 4 polish)
        // ============================================
        void UpdateParticles(float dt)
        {
            for (auto& particle : m_Particles)
            {
                if (particle.Life <= 0.0f)
                    continue;

                particle.Life -= dt;
                if (particle.Life <= 0.0f)
                    continue;

                particle.Position += particle.Velocity * dt;
                particle.Rotation += particle.AngularVelocity * dt;
            }

            m_Particles.erase(std::remove_if(
                m_Particles.begin(), m_Particles.end(),
                [](const PrettyParticle& p) { return p.Life <= 0.0f; }),
                m_Particles.end());
        }

        void EmitTrailParticles(const glm::vec2& pos, const glm::vec2& velocity, float dt)
        {
            if (m_BallHidden)
                return;

            float speed = glm::length(velocity);
            if (speed < 0.4f)
                return;

            m_TrailAccumulator += dt * glm::clamp(speed, 1.0f, 14.0f);
            const float spawnInterval = 0.04f;

            while (m_TrailAccumulator >= spawnInterval)
            {
                m_TrailAccumulator -= spawnInterval;

                glm::vec2 dir = speed > 0.001f ? glm::normalize(-velocity) : glm::vec2(0.0f);
                glm::vec2 jitter(RandomRange(-0.25f, 0.25f), RandomRange(-0.25f, 0.25f));
                glm::vec2 vel = dir * RandomRange(0.5f, 2.5f) + jitter;

                glm::vec4 startColor = {0.7f, 0.95f, 1.0f, 0.8f};
                glm::vec4 endColor = {0.2f, 0.55f, 0.95f, 0.0f};

                float sizeStart = RandomRange(0.18f, 0.3f);
                float sizeEnd = 0.04f;

                SpawnParticle(
                    pos,
                    vel,
                    RandomRange(0.45f, 0.75f),
                    sizeStart,
                    sizeEnd,
                    startColor,
                    endColor,
                    RandomRange(0.0f, 1.2f),
                    RandomRange(-2.5f, 2.5f)
                );
            }
        }

        void EmitGoalBurst(const glm::vec2& pos)
        {
            for (int i = 0; i < 70; ++i)
            {
                float angle = RandomRange(0.0f, glm::two_pi<float>());
                glm::vec2 dir(std::cos(angle), std::sin(angle));
                glm::vec2 vel = dir * RandomRange(4.0f, 8.0f);

                glm::vec4 startColor = {1.0f, 0.93f, 0.45f, 0.95f};
                glm::vec4 endColor = {1.0f, 0.45f, 0.25f, 0.0f};

                SpawnParticle(
                    pos + dir * 0.15f,
                    vel,
                    RandomRange(0.6f, 1.1f),
                    RandomRange(0.22f, 0.35f),
                    0.03f,
                    startColor,
                    endColor,
                    RandomRange(0.0f, 1.5f),
                    RandomRange(-3.0f, 3.0f)
                );
            }
        }

        void EmitWellAura(float dt)
        {
            if (m_GravityWells.empty())
                return;

            m_WellPulseAccumulator += dt;
            const float interval = 0.08f;
            if (m_WellPulseAccumulator < interval)
                return;

            m_WellPulseAccumulator = 0.0f;

            for (auto& wellEntity : m_GravityWells)
            {
                auto& transform = wellEntity.GetComponent<Pillar::TransformComponent>();
                auto& well = wellEntity.GetComponent<GravityWellComponent>();

                for (int i = 0; i < 3; ++i)
                {
                    float angle = RandomRange(0.0f, glm::two_pi<float>());
                    float radius = well.Radius * RandomRange(0.25f, 1.0f);
                    glm::vec2 offset(std::cos(angle), std::sin(angle));
                    offset *= radius * 0.5f;

                    glm::vec2 tangent(-offset.y, offset.x);
                    glm::vec2 vel = glm::normalize(tangent) * RandomRange(0.5f, 2.2f);
                    if (well.IsRepulsor)
                        vel *= -1.0f;

                    glm::vec4 startColor = well.IsRepulsor
                        ? glm::vec4(1.0f, 0.45f, 0.85f, 0.85f)
                        : glm::vec4(0.4f, 1.0f, 1.0f, 0.85f);
                    glm::vec4 endColor = {startColor.r, startColor.g, startColor.b, 0.0f};

                    SpawnParticle(
                        glm::vec2(transform.Position.x, transform.Position.y) + offset,
                        vel,
                        RandomRange(0.7f, 1.1f),
                        RandomRange(0.18f, 0.32f),
                        0.04f,
                        startColor,
                        endColor,
                        RandomRange(0.0f, 0.6f),
                        RandomRange(-1.0f, 1.0f)
                    );
                }
            }
        }

        void SpawnParticle(
            const glm::vec2& position,
            const glm::vec2& velocity,
            float lifetime,
            float sizeStart,
            float sizeEnd,
            const glm::vec4& colorStart,
            const glm::vec4& colorEnd,
            float rotation,
            float angularVelocity)
        {
            if (m_Particles.size() >= m_MaxParticles)
            {
                // Drop the oldest to keep memory bounded
                m_Particles.erase(m_Particles.begin());
            }

            PrettyParticle p{};
            p.Position = position;
            p.Velocity = velocity;
            p.ColorStart = colorStart;
            p.ColorEnd = colorEnd;
            p.SizeStart = sizeStart;
            p.SizeEnd = sizeEnd;
            p.Rotation = rotation;
            p.AngularVelocity = angularVelocity;
            p.Life = lifetime;
            p.MaxLife = lifetime;
            m_Particles.push_back(p);
        }

        float RandomRange(float min, float max)
        {
            float t = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
            return min + (max - min) * t;
        }

        int CalculateStars(int shots, int par)
        {
            if (shots <= par - 1) return 3;  // Under par
            if (shots == par) return 2;       // At par
            if (shots <= par + 1) return 1;   // One over par
            return 0;                          // Way over
        }

        // ============================================
        // Main Menu Helpers
        // ============================================
        void InitMenuParticles()
        {
            m_MenuParticles.clear();
            for (size_t i = 0; i < m_MaxMenuParticles; ++i)
            {
                MenuParticle p;
                p.Position = {RandomRange(-12.0f, 12.0f), RandomRange(-8.0f, 8.0f)};
                p.Velocity = {RandomRange(-0.3f, 0.3f), RandomRange(-0.3f, 0.3f)};
                p.Size = RandomRange(0.1f, 0.4f);
                p.Alpha = RandomRange(0.1f, 0.35f);
                p.Rotation = RandomRange(0.0f, glm::two_pi<float>());
                p.RotationSpeed = RandomRange(-0.5f, 0.5f);
                
                // Some particles are gravity wells, some are balls
                p.IsGravityWell = (i % 5 == 0);
                if (p.IsGravityWell)
                {
                    p.Size = RandomRange(0.6f, 1.2f);
                    p.Alpha = RandomRange(0.15f, 0.25f);
                    // Cyan or magenta tint for wells
                    if (i % 2 == 0)
                        p.Color = {0.3f, 0.8f, 1.0f, 1.0f};
                    else
                        p.Color = {1.0f, 0.4f, 0.8f, 1.0f};
                }
                else
                {
                    p.Color = {1.0f, 1.0f, 1.0f, 1.0f};
                }
                
                m_MenuParticles.push_back(p);
            }
        }

        void UpdateMenuParticles(float dt)
        {
            for (auto& p : m_MenuParticles)
            {
                p.Position += p.Velocity * dt;
                p.Rotation += p.RotationSpeed * dt;
                
                // Wrap around screen bounds
                if (p.Position.x < -14.0f) p.Position.x = 14.0f;
                if (p.Position.x > 14.0f) p.Position.x = -14.0f;
                if (p.Position.y < -10.0f) p.Position.y = 10.0f;
                if (p.Position.y > 10.0f) p.Position.y = -10.0f;
                
                // Gentle pulsing alpha
                float pulse = 0.5f + 0.5f * std::sin(m_MenuAnimTime * 2.0f + p.Rotation);
                p.Alpha = glm::mix(0.1f, 0.35f, pulse);
            }
        }

        void RenderMenuBackground()
        {
            // Clear with dark background
            Pillar::Renderer::SetClearColor({0.02f, 0.04f, 0.06f, 1.0f});
            Pillar::Renderer::Clear();

            Pillar::Renderer2DBackend::ResetStats();
            Pillar::Renderer2DBackend::BeginScene(m_CameraController.GetCamera());

            // Draw gradient background using quads
            const float bgWidth = 30.0f;
            const float bgHeight = 20.0f;
            
            // Dark gradient from top to bottom
            Pillar::Renderer2DBackend::DrawQuad(
                {0.0f, 0.0f, -0.9f},
                {bgWidth, bgHeight},
                {0.03f, 0.06f, 0.10f, 1.0f},
                nullptr
            );

            // Draw floating particles
            for (const auto& p : m_MenuParticles)
            {
                if (p.IsGravityWell)
                {
                    // Draw pulsing gravity well rings
                    float pulse = 0.5f + 0.5f * std::sin(m_MenuAnimTime * 3.0f + p.Rotation);
                    float outerSize = p.Size * (1.0f + pulse * 0.3f);
                    
                    // Outer glow
                    glm::vec4 glowColor = p.Color;
                    glowColor.a = p.Alpha * 0.3f;
                    Pillar::Renderer2DBackend::DrawQuad(
                        {p.Position.x, p.Position.y, -0.8f},
                        {outerSize * 2.0f, outerSize * 2.0f},
                        glowColor,
                        nullptr
                    );
                    
                    // Core
                    glm::vec4 coreColor = p.Color;
                    coreColor.a = p.Alpha * 0.6f;
                    Pillar::Renderer2DBackend::DrawQuad(
                        {p.Position.x, p.Position.y, -0.7f},
                        {outerSize, outerSize},
                        coreColor,
                        nullptr
                    );
                }
                else
                {
                    // Draw small floating golf balls/dots
                    glm::vec4 ballColor = {0.9f, 0.95f, 1.0f, p.Alpha};
                    if (m_TextureBall)
                    {
                        Pillar::Renderer2DBackend::DrawQuad(
                            {p.Position.x, p.Position.y, -0.6f},
                            {p.Size, p.Size},
                            ballColor,
                            m_TextureBall
                        );
                    }
                    else
                    {
                        Pillar::Renderer2DBackend::DrawQuad(
                            {p.Position.x, p.Position.y, -0.6f},
                            {p.Size, p.Size},
                            ballColor,
                            nullptr
                        );
                    }
                }
            }

            // Draw decorative lines/grid
            const float gridAlpha = 0.04f + 0.02f * std::sin(m_MenuAnimTime * 0.5f);
            for (float x = -15.0f; x <= 15.0f; x += 2.0f)
            {
                Pillar::Renderer2DBackend::DrawQuad(
                    {x, 0.0f, -0.85f},
                    {0.02f, 20.0f},
                    {0.3f, 0.6f, 0.9f, gridAlpha},
                    nullptr
                );
            }
            for (float y = -10.0f; y <= 10.0f; y += 2.0f)
            {
                Pillar::Renderer2DBackend::DrawQuad(
                    {0.0f, y, -0.85f},
                    {30.0f, 0.02f},
                    {0.3f, 0.6f, 0.9f, gridAlpha},
                    nullptr
                );
            }

            Pillar::Renderer2DBackend::EndScene();
        }

        void StartGame(int levelIndex = 0)
        {
            m_ShowMainMenu = false;
            m_GameState = GameState::Aiming;
            // Set camera to half zoom for gameplay
            m_CameraController.SetZoomLevel(5.0f);
            LoadLevel(levelIndex);
        }

        void ReturnToMainMenu()
        {
            CleanupLevel();
            m_ShowMainMenu = true;
            m_GameState = GameState::MainMenu;
            m_MenuAnimTime = 0.0f;
            // Set camera back to max zoom out for main menu
            m_CameraController.SetZoomLevel(8.5f);
            InitMenuParticles();
        }

        void RenderMainMenu()
        {
            EnsureUIStyle();

            ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImVec2 origin = viewport->WorkPos;
            ImVec2 area = viewport->WorkSize;
            ImVec2 center(origin.x + area.x * 0.5f, origin.y + area.y * 0.5f);

            // Color palette
            const ImVec4 accentCyan(0.30f, 0.75f, 1.0f, 1.0f);
            const ImVec4 accentMagenta(0.95f, 0.45f, 0.75f, 1.0f);
            const ImVec4 textWhite(0.95f, 0.97f, 1.0f, 1.0f);
            const ImVec4 textMuted(0.55f, 0.62f, 0.72f, 1.0f);
            const ImVec4 panelBg(0.06f, 0.08f, 0.12f, 0.95f);
            const ImVec4 buttonBg(0.12f, 0.15f, 0.22f, 1.0f);
            const ImVec4 buttonHover(0.18f, 0.25f, 0.38f, 1.0f);
            const ImVec4 buttonActive(0.25f, 0.35f, 0.50f, 1.0f);

            // Load title font if needed (larger size)
            if (!m_TitleFont)
            {
                ImGuiIO& io = ImGui::GetIO();
                ImFontConfig cfg;
                cfg.OversampleH = 2;
                cfg.OversampleV = 2;
                
                const std::vector<std::string> fontCandidates = {
                    Pillar::AssetManager::GetAssetPath("fonts/Inter-Bold.ttf"),
                    Pillar::AssetManager::GetAssetPath("fonts/Inter-SemiBold.ttf"),
                    Pillar::AssetManager::GetAssetPath("fonts/Manrope-Bold.ttf"),
                    "C:/Windows/Fonts/segoeuib.ttf",
                    "C:/Windows/Fonts/verdanab.ttf"
                };
                
                for (const auto& path : fontCandidates)
                {
                    if (!path.empty())
                    {
                        m_TitleFont = io.Fonts->AddFontFromFileTTF(path.c_str(), 48.0f, &cfg);
                        if (m_TitleFont)
                            break;
                    }
                }
                if (!m_TitleFont)
                    m_TitleFont = io.Fonts->AddFontDefault();
            }

            // Main menu window - centered
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowBgAlpha(0.0f);

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));

            if (ImGui::Begin("##MainMenu", nullptr,
                ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_AlwaysAutoResize |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoSavedSettings |
                ImGuiWindowFlags_NoBackground))
            {
                // Title with glow effect
                ImGui::PushFont(m_TitleFont ? m_TitleFont : ImGui::GetFont());
                
                // Animated title color
                float hue = std::fmod(m_MenuAnimTime * 0.1f, 1.0f);
                float pulse = 0.5f + 0.5f * std::sin(m_MenuAnimTime * 2.0f);
                ImVec4 titleColor = ImVec4(
                    glm::mix(0.30f, 0.40f, pulse),
                    glm::mix(0.75f, 0.90f, pulse),
                    1.0f,
                    1.0f
                );

                // Center the title
                const char* title = "GRAVITY GOLF";
                ImVec2 titleSize = ImGui::CalcTextSize(title);
                float windowWidth = 400.0f;
                ImGui::SetCursorPosX((windowWidth - titleSize.x) * 0.5f);
                
                ImGui::TextColored(titleColor, "%s", title);
                ImGui::PopFont();

                // Subtitle
                ImGui::PushFont(m_UiFont ? m_UiFont : ImGui::GetFont());
                const char* subtitle = "A physics puzzle game";
                ImVec2 subtitleSize = ImGui::CalcTextSize(subtitle);
                ImGui::SetCursorPosX((windowWidth - subtitleSize.x) * 0.5f);
                ImGui::TextColored(textMuted, "%s", subtitle);
                
                ImGui::Spacing();
                ImGui::Spacing();
                ImGui::Spacing();

                // Menu panel
                ImGui::PushStyleColor(ImGuiCol_ChildBg, panelBg);
                ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 16.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(40.0f, 30.0f));
                
                if (ImGui::BeginChild("##MenuPanel", ImVec2(windowWidth, 350.0f), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
                {
                    // Button styling
                    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(20.0f, 16.0f));
                    ImGui::PushStyleColor(ImGuiCol_Button, buttonBg);
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, buttonHover);
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, buttonActive);

                    float buttonWidth = windowWidth - 80.0f;
                    float buttonHeight = 50.0f;

                    // Play button
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.45f, 0.35f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.55f, 0.42f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.25f, 0.65f, 0.50f, 1.0f));
                    if (ImGui::Button("PLAY", ImVec2(buttonWidth, buttonHeight)))
                    {
                        StartGame(0);
                    }
                    ImGui::PopStyleColor(3);

                    ImGui::Spacing();
                    ImGui::Spacing();

                    // Level Select button
                    if (ImGui::Button("SELECT LEVEL", ImVec2(buttonWidth, buttonHeight)))
                    {
                        m_ShowLevelSelect = true;
                    }

                    ImGui::Spacing();
                    ImGui::Spacing();

                    // Settings / Sound toggle
                    const bool isMuted = m_IsMuted || m_Audio.GetMasterVolume() <= 0.001f;
                    const char* soundLabel = isMuted ? "SOUND: OFF" : "SOUND: ON";
                    
                    ImVec4 soundBtnColor = isMuted 
                        ? ImVec4(0.35f, 0.18f, 0.18f, 1.0f)
                        : ImVec4(0.12f, 0.15f, 0.22f, 1.0f);
                    ImVec4 soundBtnHover = isMuted
                        ? ImVec4(0.45f, 0.22f, 0.22f, 1.0f)
                        : ImVec4(0.18f, 0.25f, 0.38f, 1.0f);
                    
                    ImGui::PushStyleColor(ImGuiCol_Button, soundBtnColor);
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, soundBtnHover);
                    if (ImGui::Button(soundLabel, ImVec2(buttonWidth, buttonHeight)))
                    {
                        ToggleMute();
                    }
                    ImGui::PopStyleColor(2);

                    ImGui::Spacing();
                    ImGui::Spacing();

                    // Quit button
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.30f, 0.12f, 0.12f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.45f, 0.18f, 0.18f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.55f, 0.22f, 0.22f, 1.0f));
                    if (ImGui::Button("QUIT", ImVec2(buttonWidth, buttonHeight)))
                    {
                        auto* window = static_cast<GLFWwindow*>(Pillar::Application::Get().GetWindow().GetNativeWindow());
                        glfwSetWindowShouldClose(window, GLFW_TRUE);
                    }
                    ImGui::PopStyleColor(3);

                    ImGui::PopStyleColor(3); // Button colors
                    ImGui::PopStyleVar(2);   // Frame rounding/padding
                }
                ImGui::EndChild();
                ImGui::PopStyleVar(2);   // Child rounding/padding
                ImGui::PopStyleColor();  // ChildBg

                // Version/credits at bottom
                ImGui::Spacing();
                const char* version = "v1.0 - Made with Pillar Engine";
                ImVec2 versionSize = ImGui::CalcTextSize(version);
                ImGui::SetCursorPosX((windowWidth - versionSize.x) * 0.5f);
                ImGui::TextColored(ImVec4(0.4f, 0.45f, 0.5f, 0.7f), "%s", version);

                ImGui::PopFont();
            }
            ImGui::End();

            ImGui::PopStyleColor();
            ImGui::PopStyleVar(2);

            // Render level select popup if open (reuse existing function but handle menu context)
            if (m_ShowLevelSelect)
            {
                RenderMenuLevelSelect();
            }
        }

        void RenderMenuLevelSelect()
        {
            ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImVec2 origin = viewport->WorkPos;
            ImVec2 area = viewport->WorkSize;
            ImVec2 center(origin.x + area.x * 0.5f, origin.y + area.y * 0.5f);

            const ImVec4 panelBg(0.06f, 0.08f, 0.12f, 0.98f);
            const ImVec4 accentCyan(0.30f, 0.75f, 1.0f, 1.0f);
            const ImVec4 textMuted(0.55f, 0.62f, 0.72f, 1.0f);

            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowBgAlpha(0.98f);

            ImGui::PushFont(m_UiFont ? m_UiFont : ImGui::GetFont());
            ImGui::PushStyleColor(ImGuiCol_WindowBg, panelBg);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 16.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(30.0f, 25.0f));

            if (ImGui::Begin("##MenuLevelSelect", &m_ShowLevelSelect,
                ImGuiWindowFlags_AlwaysAutoResize |
                ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoSavedSettings))
            {
                ImGui::TextColored(accentCyan, "SELECT LEVEL");
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(15.0f, 10.0f));

                for (size_t i = 0; i < m_Levels.size(); ++i)
                {
                    const auto& level = m_Levels[i];
                    int best = (i < m_BestShots.size()) ? m_BestShots[i] : std::numeric_limits<int>::max();
                    int stars = best == std::numeric_limits<int>::max() ? 0 : CalculateStars(best, level.Par);

                    ImGui::PushID(static_cast<int>(i));

                    // Level card
                    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.10f, 0.12f, 0.18f, 1.0f));
                    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 10.0f);
                    
                    if (ImGui::BeginChild("##LevelCard", ImVec2(350.0f, 70.0f), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
                    {
                        ImGui::BeginGroup();
                        
                        const char* levelName = level.Name ? level.Name : "Level";
                        ImGui::TextColored(ImVec4(0.95f, 0.97f, 1.0f, 1.0f), "%d. %s", level.LevelNumber, levelName);
                        
                        // Stars display
                        ImVec4 starColor(0.95f, 0.85f, 0.30f, 1.0f);
                        ImVec4 emptyStarColor(0.30f, 0.32f, 0.38f, 1.0f);
                        ImGui::Text("Par %d  ", level.Par);
                        ImGui::SameLine();
                        for (int s = 0; s < 3; ++s)
                        {
                            ImGui::TextColored(s < stars ? starColor : emptyStarColor, "*");
                            if (s < 2) ImGui::SameLine(0.0f, 2.0f);
                        }
                        
                        ImGui::EndGroup();
                        ImGui::SameLine(260.0f);

                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.40f, 0.32f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.50f, 0.40f, 1.0f));
                        if (ImGui::Button("Play", ImVec2(70.0f, 40.0f)))
                        {
                            StartGame(static_cast<int>(i));
                            m_ShowLevelSelect = false;
                        }
                        ImGui::PopStyleColor(2);
                    }
                    ImGui::EndChild();
                    
                    ImGui::PopStyleVar();
                    ImGui::PopStyleColor();
                    ImGui::PopID();
                    
                    ImGui::Spacing();
                }

                ImGui::PopStyleVar(2);

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // Back button
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.15f, 0.15f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.20f, 0.20f, 1.0f));
                if (ImGui::Button("BACK", ImVec2(120.0f, 40.0f)))
                {
                    m_ShowLevelSelect = false;
                }
                ImGui::PopStyleColor(2);
            }
            ImGui::End();

            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor();
            ImGui::PopFont();
        }

    private:
        enum class IconSlot
        {
            Restart,
            Star,
            Mute,
            Hand
        };

        struct BoostPadRuntime
        {
            Pillar::Entity Entity;
        };

        struct MovingPlatformRuntime
        {
            Pillar::Entity Entity;
        };

        struct IconUV
        {
            ImVec2 Min;
            ImVec2 Max;
        };

        struct PrettyParticle
        {
            glm::vec2 Position{0.0f};
            glm::vec2 Velocity{0.0f};
            glm::vec4 ColorStart{1.0f};
            glm::vec4 ColorEnd{0.0f};
            float SizeStart = 0.2f;
            float SizeEnd = 0.05f;
            float Rotation = 0.0f;
            float AngularVelocity = 0.0f;
            float Life = 0.0f;
            float MaxLife = 0.0f;
        };

        IconUV GetIconUV(IconSlot slot) const
        {
            // Texture is split into four quadrants; restart/star/hand are sampled upside-down for a playful tilt
            constexpr float half = 0.5f;
            switch (slot)
            {
                case IconSlot::Restart: return {ImVec2(0.0f, 1.0f), ImVec2(half, half)}; // flipped vertically
                case IconSlot::Star:    return {ImVec2(half, 1.0f), ImVec2(1.0f, half)}; // flipped vertically
                case IconSlot::Mute:    return {ImVec2(0.0f, 0.0f), ImVec2(half, half)}; // unchanged
                case IconSlot::Hand:    return {ImVec2(half, half), ImVec2(1.0f, 0.0f)}; // flipped vertically
            }
            return {ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f)};
        }

        void DrawIconButton(const char* id, IconSlot slot, const char* tooltip, const ImVec2& size, const ImVec4& tint, std::function<void()> onClick)
        {
            if (!m_TextureIcons)
                return;

            ImTextureRef texRef(static_cast<ImTextureID>(m_TextureIcons->GetRendererID()));
            IconUV uv = GetIconUV(slot);

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 0.1f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1, 1, 1, 0.15f));

            if (ImGui::ImageButton(id, texRef, size, uv.Min, uv.Max, ImVec4(0,0,0,0), tint))
            {
                if (onClick)
                    onClick();
            }

            if (tooltip)
                ImGui::SetItemTooltip("%s", tooltip);

            ImGui::PopStyleColor(3);
            ImGui::PopStyleVar();
        }

        // Scene & Systems
        std::unique_ptr<Pillar::Scene> m_Scene;
        Pillar::PhysicsSystem* m_PhysicsSystem = nullptr;
        Pillar::PhysicsSyncSystem* m_PhysicsSyncSystem = nullptr;

        // Camera
        Pillar::OrthographicCameraController m_CameraController;

        // Game entities
        Pillar::Entity m_BallEntity;
        Pillar::Entity m_GoalEntity;
        std::vector<Pillar::Entity> m_GravityWells;
        std::vector<BoostPadRuntime> m_BoostPads;
        std::vector<MovingPlatformRuntime> m_MovingPlatforms;

        // Level data
        std::vector<LevelData> m_Levels;
        int m_CurrentLevelIndex = 0;
        std::vector<int> m_BestShots;

        // Game state
        GameState m_GameState = GameState::Aiming;
        int m_ShotCount = 0;
        bool m_ShowLevelComplete = false;
        float m_LevelCompleteTimer = 0.0f;
        bool m_RestartKeyHeld = false;
        bool m_BallHidden = false;
        bool m_IsMuted = false;
        float m_LastVolume = 1.0f;
        bool m_ShowHelp = false;
        bool m_ShowLevelSelect = false;
        float m_Time = 0.0f;

        // Wall rendering/physics constants
        static constexpr glm::vec2 DominoSize = {1.25f, 0.55f};
        static constexpr float WallEdgeInset = 0.08f;
        static constexpr float ShadowOffset = 0.05f;
        static constexpr float ShadowAlpha = 0.32f;

        // Particles
        std::vector<PrettyParticle> m_Particles;
        float m_TrailAccumulator = 0.0f;
        float m_WellPulseAccumulator = 0.0f;
        static constexpr size_t m_MaxParticles = 900;

        // Audio
        GameAudio m_Audio;
        std::unique_ptr<b2ContactListener> m_ContactListener;

        // UI
        ImFont* m_UiFont = nullptr;
        ImFont* m_TitleFont = nullptr;
        bool m_StyleInitialized = false;

        // UI textures
        std::shared_ptr<Pillar::Texture2D> m_TextureIcons;
        std::shared_ptr<Pillar::Texture2D> m_TextureBooster;

        // Textures
        std::shared_ptr<Pillar::Texture2D> m_TextureGrass;
        std::shared_ptr<Pillar::Texture2D> m_TextureWall;
        std::shared_ptr<Pillar::Texture2D> m_TextureGoal;
        std::shared_ptr<Pillar::Texture2D> m_TextureBall;

        // Main Menu state
        bool m_ShowMainMenu = true;
        float m_MenuAnimTime = 0.0f;
        int m_HoveredButton = -1;
        
        // Menu particles for animated background
        struct MenuParticle
        {
            glm::vec2 Position{0.0f};
            glm::vec2 Velocity{0.0f};
            float Size = 0.3f;
            float Alpha = 0.5f;
            float RotationSpeed = 0.0f;
            float Rotation = 0.0f;
            glm::vec4 Color{1.0f};
            bool IsGravityWell = false;
        };
        std::vector<MenuParticle> m_MenuParticles;
        static constexpr size_t m_MaxMenuParticles = 50;
    };

} // namespace BallGame
