#pragma once

#include "Pillar.h"
#include "Pillar/Renderer/Renderer2D.h"
#include "Pillar/Audio/AudioClip.h"
#include "Pillar/Audio/AudioEngine.h"
#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <vector>
#include <string>
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <algorithm>

// ===========================================
// SWARM SLAYER - Ported to Pillar Engine
// Original: Raylib Top Down Shooter
// ===========================================

struct ImGuiDisabledScope
{
    bool Disabled = false;
    ImGuiDisabledScope(bool disabled)
        : Disabled(disabled)
    {
        if (!Disabled)
            return;

        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
    }

    ~ImGuiDisabledScope()
    {
        if (!Disabled)
            return;

        ImGui::PopStyleVar();
        ImGui::PopItemFlag();
    }
};

namespace SwarmSlayer
{
    static glm::vec4 CenteredRect(const glm::vec2& center, const glm::vec2& size)
    {
        glm::vec2 half = size * 0.5f;
        return { center.x - half.x, center.y - half.y, size.x, size.y };
    }

    static glm::vec4 CenteredRect(const glm::vec2& center, float size)
    {
        return CenteredRect(center, { size, size });
    }

    // Timer utility
    struct Timer
    {
        float Lifetime = 0.0f;

        void Start(float lifetime) { Lifetime = lifetime; }
        void Update(float dt)
        {
            if (Lifetime <= 0.0f)
                return;

            Lifetime -= dt;
            if (Lifetime < 0.0f)
                Lifetime = 0.0f;
        }
        bool IsDone() const { return Lifetime <= 0; }
    };

    static glm::vec2 SafeNormalize(const glm::vec2& v, const glm::vec2& fallback = { 0.0f, 1.0f })
    {
        float len = glm::length(v);
        if (len <= 0.0001f)
            return fallback;
        return v / len;
    }

    // Player spritesheet assumptions (adjust if you swap the sheet)
    static constexpr int PlayerColumns = 8;
    static constexpr int PlayerRows = 9;
    static constexpr int PlayerIdleFrames = 2;
    static constexpr int PlayerWalkFrames = 8;
    // Some sheets have a blank 8th death frame; keep this at 7 to avoid sampling empties.
    static constexpr int PlayerDeathFrames = 7;
    static constexpr int PlayerIdleRow = 0;
    static constexpr int PlayerWalkRow = 3;
    static constexpr int PlayerDeathRow = 7;

    // Game state enum
    enum class GameState { Menu, Playing, Dead };
    enum class PlayerState { Alive, Dead };

    // Experience points (coins)
    struct ExpOrb
    {
        glm::vec2 Position;
        glm::vec2 Size;
        float ExpValue;
        int Frame = 0;
        Timer AnimTimer;
        int CoinType; // 0=bronze, 1=silver, 2=gold

        ExpOrb(glm::vec2 pos, float exp)
            : Position(pos), ExpValue(exp)
        {
            if (exp < 50)
            {
                CoinType = 0; // bronze
                Size = glm::vec2(exp < 25 ? 0.3f : 0.4f);
            }
            else if (exp < 100)
            {
                CoinType = 1; // silver
                Size = glm::vec2(exp < 75 ? 0.3f : 0.4f);
            }
            else
            {
                CoinType = 2; // gold
                Size = glm::vec2(exp < 125 ? 0.3f : 0.4f);
            }
        }

        void Update(float dt)
        {
            AnimTimer.Update(dt);
            if (AnimTimer.IsDone())
            {
                AnimTimer.Start(0.3f);
                Frame = (Frame + 1) % 7;
            }
        }

        glm::vec4 GetRect() const
        {
            return CenteredRect(Position, Size);
        }
    };

    // Bullet
    struct Bullet
    {
        glm::vec2 Position;
        glm::vec2 Direction;
        float Angle;
        Timer RangeTimer;
        bool Active = true;
        float Speed;

        Bullet(glm::vec2 pos, glm::vec2 dir, float speed)
            : Position(pos), Direction(dir), Speed(speed)
        {
            Angle = -glm::degrees(std::atan2(dir.y, dir.x)) - 90.0f;
            RangeTimer.Start(5.0f);
        }

        void Update(float dt)
        {
            if (!RangeTimer.IsDone())
            {
                Position += Direction * Speed * dt;
                RangeTimer.Update(dt);
            }
            else
            {
                Active = false;
            }
        }

        glm::vec4 GetRect() const
        {
            return CenteredRect(Position, 0.6f);
        }
    };

    // Enemy types
    enum class EnemyType { Goblin, Orbie, Hoodzy };

    struct EnemyDef
    {
        std::string Name;
        float Health;
        float Speed;
        float Damage;
        float ExpValue;
        int AnimFrames;
    };

    struct Enemy
    {
        glm::vec2 Position;
        glm::vec2 Size = { 0.8f, 0.8f };
        float Health;
        float MaxHealth;
        float Speed;
        float Damage;
        float ExpValue;
        EnemyType Type;
        Timer HitStun;
        Timer PlayerHitStun;
        Timer AnimTimer;
        int AnimFrame = 0;
        int AnimFrameCount;
        glm::vec2 KnockbackDir;
        bool FacingLeft = false;

        Enemy(glm::vec2 pos, EnemyType type, float health, float speed, float damage, float exp, int animFrames)
            : Position(pos), Type(type), Health(health), MaxHealth(health),
              Speed(speed), Damage(damage), ExpValue(exp), AnimFrameCount(animFrames) {}

        void Update(float dt, glm::vec2 playerPos)
        {
            HitStun.Update(dt);
            AnimTimer.Update(dt);
            PlayerHitStun.Update(dt);

            if (HitStun.IsDone())
            {
                glm::vec2 dir = SafeNormalize(playerPos - Position);
                Position += dir * Speed * dt;
            }
            else
            {
                Position += KnockbackDir * HitStun.Lifetime;
            }

            // Animation
            if (AnimTimer.IsDone())
            {
                AnimTimer.Start(0.1f);
                AnimFrame = (AnimFrame + 1) % AnimFrameCount;
            }

            FacingLeft = playerPos.x < Position.x;
        }

        glm::vec4 GetRect() const
        {
            return CenteredRect(Position, Size);
        }

        void TakeDamage(float damage, glm::vec2 bulletDir, float knockback)
        {
            Health -= damage;
            HitStun.Start(0.5f);
            KnockbackDir = bulletDir * knockback * 0.016f;
        }
    };

    // Upgrade
    struct Upgrade
    {
        std::string Name;
        int Level = 0;
        int MaxLevel = 8;

        int GetCost() const { return 40 * (Level + 1); }
        bool CanAfford(float exp) const { return exp >= GetCost() && Level < MaxLevel; }
    };

    // Player
    struct Player
    {
        glm::vec2 Position = { 5.0f, 5.0f };
        glm::vec2 Size = { 0.8f, 0.8f };
        float Health = 100.0f;
        float MaxHealth = 100.0f;
        float Speed = 2.0f;
        float Exp = 0.0f;
        PlayerState State = PlayerState::Alive;
        Timer AnimTimer;
        int AnimFrame = 0;
        bool IsMoving = false;
        bool FacingLeft = false;
        Timer DeathAnimTimer;
        int DeathFrame = 0;
        bool ShouldPlayDeathSound = false;

        // Gun stats
        float GunDamage = 20.0f;
        int Ammo = 10;
        int MaxAmmo = 10;
        float ReloadTime = 2.0f;
        Timer ReloadTimer;
        float BulletSpeed = 8.0f;
        float Knockback = 0.8f;

        // Upgrades
        std::vector<Upgrade> Upgrades;

        Player()
        {
            Upgrades.push_back({"Speed", 0, 8});
            Upgrades.push_back({"Knockback", 0, 8});
            Upgrades.push_back({"Damage", 0, 8});
            Upgrades.push_back({"Health", 0, 8});
            Upgrades.push_back({"Reload", 0, 8});
            Upgrades.push_back({"Ammo", 0, 8});
        }

        void Update(float dt)
        {
            if (State == PlayerState::Alive)
            {
                AnimTimer.Update(dt);
                bool wasReloading = ReloadTimer.Lifetime > 0.0f;
                ReloadTimer.Update(dt);
                if (wasReloading && ReloadTimer.IsDone())
                {
                    Ammo = MaxAmmo;
                }

                IsMoving = false;
                if (Pillar::Input::IsKeyPressed(PIL_KEY_W)) { Position.y += Speed * dt; IsMoving = true; }
                if (Pillar::Input::IsKeyPressed(PIL_KEY_S)) { Position.y -= Speed * dt; IsMoving = true; }
                if (Pillar::Input::IsKeyPressed(PIL_KEY_D)) { Position.x += Speed * dt; IsMoving = true; }
                if (Pillar::Input::IsKeyPressed(PIL_KEY_A)) { Position.x -= Speed * dt; IsMoving = true; }

                // Walk animation
                if (AnimTimer.IsDone())
                {
                    AnimTimer.Start(0.2f);
                    AnimFrame = (AnimFrame + 1) % (IsMoving ? PlayerWalkFrames : PlayerIdleFrames);
                }

                if (Health <= 0)
                {
                    State = PlayerState::Dead;
                    DeathFrame = 0;
                    DeathAnimTimer.Start(0.5f);
                    ShouldPlayDeathSound = true;
                }
            }
            else
            {
                DeathAnimTimer.Update(dt);
                if (DeathAnimTimer.IsDone() && DeathFrame < (PlayerDeathFrames - 1))
                {
                    DeathAnimTimer.Start(0.5f);
                    DeathFrame++;
                }
            }
        }

        glm::vec4 GetRect() const
        {
            return CenteredRect(Position, Size);
        }

        void TryReload()
        {
            if (Ammo < MaxAmmo && ReloadTimer.IsDone())
            {
                ReloadTimer.Start(ReloadTime);
            }
        }

        void ApplyUpgrade(int index)
        {
            if (index >= 0 && index < (int)Upgrades.size() && Upgrades[index].CanAfford(Exp))
            {
                Exp -= Upgrades[index].GetCost();
                Upgrades[index].Level++;

                switch (index)
                {
                    case 0: Speed += 0.8f; break;
                    case 1: Knockback += 0.1f; break;
                    case 2: GunDamage *= 1.5f; break;
                    case 3:
                        MaxHealth *= 1.2f;
                        Health = std::min(Health + MaxHealth * 0.2f, MaxHealth);
                        break;
                    case 4: ReloadTime *= 0.8f; break;
                    case 5: MaxAmmo += 2; break;
                }
            }
        }
    };

    // Wave Manager
    struct WaveManager
    {
        int WaveNumber = 1;
        Timer DelayTimer;

        void Update(float dt, std::vector<Enemy>& enemies, Player& player)
        {
            DelayTimer.Update(dt);

            if (DelayTimer.IsDone() && enemies.empty())
            {
                SpawnWave(enemies, player);
                WaveNumber++;
                DelayTimer.Start(5.0f);
            }
        }

        void SpawnWave(std::vector<Enemy>& enemies, Player& player)
        {
            auto spawnEnemy = [&](EnemyType type, int count)
            {
                for (int i = 0; i < count; i++)
                {
                    float angle = ((float)(std::rand() % 360)) * 3.14159f / 180.0f;
                    float dist = 5.0f + (std::rand() % 5);
                    glm::vec2 pos = player.Position + glm::vec2(std::cos(angle), std::sin(angle)) * dist;

                    switch (type)
                    {
                        case EnemyType::Goblin:
                            enemies.emplace_back(pos, type, 100.0f, 1.5f, 15.0f, 30.0f, 8);
                            break;
                        case EnemyType::Orbie:
                            enemies.emplace_back(pos, type, 200.0f, 3.0f, 30.0f, 100.0f, 12);
                            break;
                        case EnemyType::Hoodzy:
                            enemies.emplace_back(pos, type, 150.0f, 4.0f, 60.0f, 150.0f, 6);
                            break;
                    }
                }
            };

            switch (WaveNumber)
            {
                case 1: spawnEnemy(EnemyType::Goblin, 5); break;
                case 2: spawnEnemy(EnemyType::Orbie, 2); break;
                case 3:
                    spawnEnemy(EnemyType::Orbie, 4);
                    spawnEnemy(EnemyType::Goblin, 5);
                    break;
                case 4: spawnEnemy(EnemyType::Hoodzy, 2); break;
                case 5:
                    spawnEnemy(EnemyType::Hoodzy, 4);
                    spawnEnemy(EnemyType::Goblin, 10);
                    break;
                default:
                    spawnEnemy(EnemyType::Hoodzy, 2 + WaveNumber / 2);
                    spawnEnemy(EnemyType::Goblin, 5 + WaveNumber);
                    spawnEnemy(EnemyType::Orbie, WaveNumber / 2);
                    break;
            }
        }
    };

    // Obstacle (Box)
    struct Obstacle
    {
        glm::vec2 Position;
        float Scale;

        glm::vec4 GetRect() const
        {
            return CenteredRect(Position, Scale);
        }
    };
}

class TopDownShooterLayer : public Pillar::Layer
{
public:
    TopDownShooterLayer()
        : Layer("TopDownShooterLayer"),
          m_CameraController(16.0f / 9.0f, false)
    {
        std::srand(static_cast<unsigned>(std::time(nullptr)));
    }

    void OnAttach() override
    {
        Layer::OnAttach();
        PIL_INFO("SwarmSlayer - Top Down Shooter loaded!");

        // Load textures
        m_PlayerTexture = Pillar::Texture2D::Create("AnimationSheet_Character.png");
        m_GrassTexture = Pillar::Texture2D::Create("Grass.png");
        m_BoxTexture = Pillar::Texture2D::Create("Box.png");
        m_BulletTexture = Pillar::Texture2D::Create("Soul_Orb.png");
        m_CoinTexture = Pillar::Texture2D::Create("Coins.png");
        m_GoblinTexture = Pillar::Texture2D::Create("e_run.png");
        m_OrbieTexture = Pillar::Texture2D::Create("p_ball.png");
        m_HoodzyTexture = Pillar::Texture2D::Create("hoodzy.png");

        // Generate map obstacles
        for (int i = 0; i < 50; i++)
        {
            float x = (std::rand() % 200 - 100) * 0.2f;
            float y = (std::rand() % 200 - 100) * 0.2f;
            // Slightly larger obstacles for better visibility / gameplay.
            float scale = 0.45f + (std::rand() % 5) * 0.15f;
            m_Obstacles.push_back({ {x, y}, scale });
        }

        m_CameraController.SetZoomLevel(5.0f);
        PIL_INFO("Controls: WASD - Move, Left Click - Shoot, R - Reload, TAB - Upgrades");

        // Load audio
        m_ShootSound = Pillar::AudioClip::Create("shoot_fireball.wav");
        if (m_ShootSound)
            m_ShootSound->SetVolume(0.1f);
        m_PlayerHurtSound = Pillar::AudioClip::Create("player_hurt.wav");
        if (m_PlayerHurtSound)
            m_PlayerHurtSound->SetVolume(0.5f);
        m_PlayerDiesSound = Pillar::AudioClip::Create("player_dies.wav");
        if (m_PlayerDiesSound)
            m_PlayerDiesSound->SetVolume(0.7f);
        m_PickupCoinSound = Pillar::AudioClip::Create("pickup_coin.wav");
        if (m_PickupCoinSound)
            m_PickupCoinSound->SetVolume(0.45f);
        m_HitEnemySound = Pillar::AudioClip::Create("fireball_hits_enemy.wav");
        if (m_HitEnemySound)
            m_HitEnemySound->SetVolume(0.35f);
        
        // Background music
        m_BackgroundMusic = Pillar::AudioClip::Create("top_down_shooter_background_loop.wav");
        if (m_BackgroundMusic)
        {
            m_BackgroundMusic->SetLooping(true);
            m_BackgroundMusic->SetVolume(0.4f);
        }
    }

    void OnDetach() override
    {
        Layer::OnDetach();
    }

    void OnUpdate(float dt) override
    {
        // Limit delta time for stability
        dt = std::min(dt, 0.05f);

        // Input (polled) so UI can't swallow key/mouse events.
        UpdatePolledInput();

        if (m_GameState == SwarmSlayer::GameState::Menu)
        {
            // Menu state - waiting for click
            // Stop music if playing
            if (m_BackgroundMusic && m_BackgroundMusic->IsPlaying())
                m_BackgroundMusic->Stop();
        }
        else if (m_GameState == SwarmSlayer::GameState::Playing)
        {
            UpdateGame(dt);
        }

        // Update player facing direction based on mouse every frame
        glm::vec2 mouseWorld = GetMouseWorldPosition();
        m_Player.FacingLeft = mouseWorld.x < m_Player.Position.x;

        // Update camera to follow player
        glm::vec3 camPos = { m_Player.Position.x, m_Player.Position.y, 0.0f };
        m_CameraController.GetCamera().SetPosition(camPos);

        RenderGame();
    }

    void UpdateGame(float dt)
    {
        m_Player.Update(dt);
        
        // Check if player just died and play death sound
        if (m_Player.ShouldPlayDeathSound)
        {
            m_Player.ShouldPlayDeathSound = false;
            if (m_PlayerDiesSound)
                m_PlayerDiesSound->Play();
            // Stop background music on death
            if (m_BackgroundMusic)
                m_BackgroundMusic->Stop();
        }

        // Auto-reload
        if (m_Player.Ammo == 0 && m_Player.ReloadTimer.IsDone())
        {
            m_Player.TryReload();
        }

        // Reload on R key
        if (Pillar::Input::IsKeyPressed(PIL_KEY_R))
        {
            m_Player.TryReload();
        }

        // Shooting (edge triggered)
        if (m_MouseLeftPressedThisFrame &&
            m_Player.State == SwarmSlayer::PlayerState::Alive &&
            !m_ShowUpgradePanel &&
            m_GameState == SwarmSlayer::GameState::Playing &&
            true)
        {
            if (m_Player.Ammo > 0 && m_Player.ReloadTimer.IsDone())
            {
                glm::vec2 mouseWorld = GetMouseWorldPosition();
                glm::vec2 dir = SwarmSlayer::SafeNormalize(mouseWorld - m_Player.Position);
                glm::vec2 spawnPos = m_Player.Position + dir * 0.8f;
                m_Bullets.emplace_back(spawnPos, dir, m_Player.BulletSpeed);
                m_Player.Ammo--;
                
                // Play shoot sound
                if (m_ShootSound)
                    m_ShootSound->Play();
            }
        }

        // Wave management
        if (m_Player.State == SwarmSlayer::PlayerState::Alive)
        {
            m_WaveManager.Update(dt, m_Enemies, m_Player);
        }

        // Update enemies
        for (auto& enemy : m_Enemies)
        {
            enemy.Update(dt, m_Player.Position);

            // Collision with player
            if (CheckCollision(enemy.GetRect(), m_Player.GetRect()))
            {
                // Push enemy away
                glm::vec2 pushDir = SwarmSlayer::SafeNormalize(enemy.Position - m_Player.Position);
                enemy.Position += pushDir * 0.05f;

                // Damage player
                if (enemy.PlayerHitStun.IsDone() && m_Player.State == SwarmSlayer::PlayerState::Alive)
                {
                    m_Player.Health -= enemy.Damage;
                    enemy.PlayerHitStun.Start(3.0f);
                    
                    // Play hurt sound
                    if (m_PlayerHurtSound)
                        m_PlayerHurtSound->Play();
                }
            }
        }

        // Enemy-enemy collision
        for (size_t i = 0; i < m_Enemies.size(); i++)
        {
            for (size_t j = i + 1; j < m_Enemies.size(); j++)
            {
                if (CheckCollision(m_Enemies[i].GetRect(), m_Enemies[j].GetRect()))
                {
                    glm::vec2 pushDir = SwarmSlayer::SafeNormalize(m_Enemies[i].Position - m_Enemies[j].Position);
                    m_Enemies[i].Position += pushDir * 0.02f;
                    m_Enemies[j].Position -= pushDir * 0.02f;
                }
            }
        }

        // Update bullets
        for (auto& bullet : m_Bullets)
        {
            bullet.Update(dt);

            // Bullet vs obstacles (stop bullets going through boxes)
            if (bullet.Active)
            {
                for (const auto& obs : m_Obstacles)
                {
                    if (CheckCollision(bullet.GetRect(), obs.GetRect()))
                    {
                        bullet.Active = false;
                        break;
                    }
                }
            }

            if (!bullet.Active)
                continue;

            // Check collision with enemies
            for (auto it = m_Enemies.begin(); it != m_Enemies.end();)
            {
                if (CheckCollision(bullet.GetRect(), it->GetRect()))
                {
                    it->TakeDamage(m_Player.GunDamage, bullet.Direction, m_Player.Knockback);
                    bullet.Active = false;

                    if (it->Health <= 0)
                    {
                        m_ExpOrbs.emplace_back(it->Position, it->ExpValue);
                        it = m_Enemies.erase(it);
                    }
                    
                    // Play hit sound
                    if (m_HitEnemySound)
                        m_HitEnemySound->Play();
                    else
                    {
                        ++it;
                    }
                    break;
                }
                else
                {
                    ++it;
                }
            }
        }

        // Remove inactive bullets
        m_Bullets.erase(
            std::remove_if(m_Bullets.begin(), m_Bullets.end(),
                [](const SwarmSlayer::Bullet& b) { return !b.Active; }),
            m_Bullets.end());

        // Update and pick up exp orbs
        for (auto it = m_ExpOrbs.begin(); it != m_ExpOrbs.end();)
        {
            it->Update(dt);
            if (CheckCollision(it->GetRect(), m_Player.GetRect()))
            {
                m_Player.Exp += it->ExpValue;
                it = m_ExpOrbs.erase(it);
                
                // Play pickup sound
                if (m_PickupCoinSound)
                    m_PickupCoinSound->Play();
            }
            else
            {
                ++it;
            }
        }

        // Obstacle collisions
        for (const auto& obs : m_Obstacles)
        {
            if (CheckCollision(m_Player.GetRect(), obs.GetRect()))
            {
                glm::vec2 pushDir = SwarmSlayer::SafeNormalize(m_Player.Position - obs.Position);
                m_Player.Position += pushDir * (m_Player.Speed + 0.5f) * dt;
            }

            for (auto& enemy : m_Enemies)
            {
                if (CheckCollision(enemy.GetRect(), obs.GetRect()))
                {
                    glm::vec2 pushDir = SwarmSlayer::SafeNormalize(enemy.Position - obs.Position);
                    enemy.Position += pushDir * (enemy.Speed + 0.5f) * dt;
                }
            }
        }

        // Confine to map bounds
        const float mapBound = 20.0f;
        m_Player.Position = glm::clamp(m_Player.Position, glm::vec2(-mapBound), glm::vec2(mapBound));
        for (auto& enemy : m_Enemies)
        {
            enemy.Position = glm::clamp(enemy.Position, glm::vec2(-mapBound), glm::vec2(mapBound));
        }
    }

    void RenderGame()
    {
        Pillar::Renderer2D::SetClearColor({ 0.1f, 0.15f, 0.2f, 1.0f });
        Pillar::Renderer2D::Clear();

        Pillar::Renderer2D::BeginScene(m_CameraController.GetCamera());

        // Draw grass tiles (slightly larger tiles)
        const float tileSize = 1.3f;
        glm::vec3 camPos = m_CameraController.GetCamera().GetPosition();
        float zoom = m_CameraController.GetZoomLevel();
        auto& window = Pillar::Application::Get().GetWindow();
        float aspect = (window.GetHeight() > 0) ? (float)window.GetWidth() / (float)window.GetHeight() : (16.0f / 9.0f);
        float viewHalfWidth = zoom * aspect;
        float viewHalfHeight = zoom;

        int startX = (int)std::floor((camPos.x - viewHalfWidth) / tileSize) - 2;
        int endX   = (int)std::ceil ((camPos.x + viewHalfWidth) / tileSize) + 2;
        int startY = (int)std::floor((camPos.y - viewHalfHeight) / tileSize) - 2;
        int endY   = (int)std::ceil ((camPos.y + viewHalfHeight) / tileSize) + 2;

        for (int x = startX; x <= endX; x++)
        {
            for (int y = startY; y <= endY; y++)
            {
                Pillar::Renderer2D::DrawQuad(
                    glm::vec3((float)x * tileSize, (float)y * tileSize, -0.5f),
                    glm::vec2(tileSize),
                    m_GrassTexture);
            }
        }

        // Draw obstacles
        for (const auto& obs : m_Obstacles)
        {
            Pillar::Renderer2D::DrawQuad(
                glm::vec3(obs.Position, -0.2f),
                glm::vec2(obs.Scale),
                m_BoxTexture);
        }

        // Draw exp orbs
        for (const auto& orb : m_ExpOrbs)
        {
            // Calculate UV for animated coin sprite
            float frameWidth = 1.0f / 7.0f;
            float rowHeight = 1.0f / 3.0f;
            int row = orb.CoinType;
            glm::vec2 uvMin = { orb.Frame * frameWidth, 1.0f - (row + 1) * rowHeight };
            glm::vec2 uvMax = { (orb.Frame + 1) * frameWidth, 1.0f - row * rowHeight };

            Pillar::Renderer2D::DrawQuad(
                glm::vec3(orb.Position, -0.1f),
                orb.Size,
                glm::vec4(1.0f),
                m_CoinTexture,
                uvMin, uvMax);
        }

        // Draw enemies
        for (const auto& enemy : m_Enemies)
        {
            std::shared_ptr<Pillar::Texture2D> tex;
            int frameCount = enemy.AnimFrameCount;
            switch (enemy.Type)
            {
                case SwarmSlayer::EnemyType::Goblin: tex = m_GoblinTexture; break;
                case SwarmSlayer::EnemyType::Orbie: tex = m_OrbieTexture; break;
                case SwarmSlayer::EnemyType::Hoodzy: tex = m_HoodzyTexture; break;
            }

            float frameWidth = 1.0f / frameCount;
            glm::vec2 uvMin = { enemy.AnimFrame * frameWidth, 0.0f };
            glm::vec2 uvMax = { (enemy.AnimFrame + 1) * frameWidth, 1.0f };

            Pillar::Renderer2D::DrawQuad(
                glm::vec3(enemy.Position, 0.0f),
                enemy.Size,
                glm::vec4(1.0f),
                tex,
                uvMin, uvMax,
                enemy.FacingLeft, false);

            // Health bar
            float healthPct = enemy.Health / enemy.MaxHealth;
            glm::vec4 healthColor = { 0.2f, 0.8f, 0.2f, 1.0f };
            if (healthPct < 0.6f) healthColor = { 1.0f, 0.6f, 0.0f, 1.0f };
            if (healthPct < 0.3f) healthColor = { 0.8f, 0.2f, 0.2f, 1.0f };

            Pillar::Renderer2D::DrawQuad(
                glm::vec3(enemy.Position.x + 0.4f, enemy.Position.y + 1.0f, 0.1f),
                glm::vec2(0.8f, 0.1f),
                { 0.3f, 0.3f, 0.3f, 0.8f });
            Pillar::Renderer2D::DrawQuad(
                glm::vec3(enemy.Position.x + 0.4f - (1.0f - healthPct) * 0.4f, enemy.Position.y + 1.0f, 0.15f),
                glm::vec2(0.8f * healthPct, 0.1f),
                healthColor);
        }

        // Draw player
        {
            int row = SwarmSlayer::PlayerIdleRow;
            int frameCount = SwarmSlayer::PlayerIdleFrames;
            int frame = m_Player.AnimFrame;

            if (m_Player.IsMoving)
            {
                row = SwarmSlayer::PlayerWalkRow;
                frameCount = SwarmSlayer::PlayerWalkFrames;
                frame = m_Player.AnimFrame;
            }

            if (m_Player.State == SwarmSlayer::PlayerState::Dead)
            {
                row = SwarmSlayer::PlayerDeathRow;
                frameCount = SwarmSlayer::PlayerDeathFrames;
                frame = m_Player.DeathFrame;
            }

            frame = std::clamp(frame, 0, frameCount - 1);

            float frameWidth = 1.0f / (float)SwarmSlayer::PlayerColumns;
            float rowHeight = 1.0f / (float)SwarmSlayer::PlayerRows;

            glm::vec2 uvMin = { frame * frameWidth, 1.0f - (row + 1) * rowHeight };
            glm::vec2 uvMax = { (frame + 1) * frameWidth, 1.0f - row * rowHeight };

            Pillar::Renderer2D::DrawQuad(
                glm::vec3(m_Player.Position, 0.1f),
                m_Player.Size,
                glm::vec4(1.0f),
                m_PlayerTexture,
                uvMin, uvMax,
                m_Player.FacingLeft, false);
        }

        // Draw bullets
        for (const auto& bullet : m_Bullets)
        {
            Pillar::Renderer2D::DrawRotatedQuad(
                glm::vec3(bullet.Position, 0.05f),
                glm::vec2(0.5f),
                glm::radians(bullet.Angle),
                { 1.0f, 0.3f, 0.3f, 1.0f },
                m_BulletTexture);
        }

        // Draw gun indicator near player
        if (m_Player.State == SwarmSlayer::PlayerState::Alive)
        {
            glm::vec2 mouseWorld = GetMouseWorldPosition();
            glm::vec2 dir = SwarmSlayer::SafeNormalize(mouseWorld - m_Player.Position);
            glm::vec2 gunPos = m_Player.Position + dir * 0.8f;
            float gunAngle = std::atan2(dir.y, dir.x);

            Pillar::Renderer2D::DrawRotatedQuad(
                glm::vec3(gunPos, 0.2f),
                glm::vec2(0.4f),
                gunAngle,
                { 0.8f, 0.8f, 1.0f, 1.0f },
                m_BulletTexture);

            // Reload bar
            if (!m_Player.ReloadTimer.IsDone())
            {
                float progress = 1.0f - (m_Player.ReloadTimer.Lifetime / m_Player.ReloadTime);
                Pillar::Renderer2D::DrawQuad(
                    glm::vec3(m_Player.Position.x + 0.4f - (1.0f - progress) * 0.4f, m_Player.Position.y - 0.5f, 0.3f),
                    glm::vec2(0.8f * progress, 0.1f),
                    { 1.0f, 1.0f, 1.0f, 0.8f });
            }
        }

        Pillar::Renderer2D::EndScene();
    }

    void OnEvent(Pillar::Event& event) override
    {
        // Keep camera resize/scroll handling working.
        m_CameraController.OnEvent(event);
    }

    void OnImGuiRender() override
    {
        // Slightly larger UI for readability.
        constexpr float uiScale = 1.20f;

        // Single "connected" UI window
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGuiWindowFlags rootFlags = ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoBackground;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 12.0f));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
        ImGui::Begin("SwarmSlayer", nullptr, rootFlags);

        // Apply base font scaling to the whole overlay window.
        ImGui::SetWindowFontScale(uiScale);

        // MENU
        if (m_GameState == SwarmSlayer::GameState::Menu)
        {
            ImVec2 content = ImGui::GetContentRegionAvail();
            ImVec2 center = ImVec2(content.x * 0.5f, content.y * 0.3f);
            ImGui::SetCursorPos(ImVec2(center.x - 200.0f, center.y - 60.0f));
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.06f, 0.06f, 0.08f, 0.92f));
            ImGui::BeginChild("MenuPanel", ImVec2(460.0f, 300.0f), true);

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
            ImGui::SetWindowFontScale(2.5f * uiScale);
            ImGui::TextUnformatted("SWARM SLAYER");
            ImGui::PopStyleColor();

            ImGui::SetWindowFontScale(1.2f * uiScale);
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Click anywhere to start!");
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Controls:");
            ImGui::BulletText("WASD - Move");
            ImGui::BulletText("Left Click - Shoot");
            ImGui::BulletText("R - Reload");
            ImGui::BulletText("TAB - Upgrades");

            ImGui::EndChild();
            ImGui::PopStyleColor();
        }
        else
        {
            // HUD (top-left)
            ImGui::SetCursorPos(ImVec2(12.0f, 12.0f));
            ImGui::BeginChild("HUDPanel", ImVec2(300.0f, 0.0f), true);

            // Make HUD text a bit larger than the base overlay scale.
            ImGui::SetWindowFontScale(1.25f * uiScale);

            float healthPct = (m_Player.MaxHealth > 0.0f) ? (m_Player.Health / m_Player.MaxHealth) : 0.0f;
            int plotColorPushCount = 0;
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
            plotColorPushCount++;
            if (healthPct < 0.6f)
            {
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(1.0f, 0.6f, 0.0f, 1.0f));
                plotColorPushCount++;
            }
            if (healthPct < 0.3f)
            {
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
                plotColorPushCount++;
            }
            ImGui::ProgressBar(healthPct, ImVec2(220.0f, 18.0f), "");
            ImGui::PopStyleColor(plotColorPushCount);

            ImGui::Text("Ammo: %d / %d", m_Player.Ammo, m_Player.MaxAmmo);
            ImGui::Text("Wave: %d", m_WaveManager.WaveNumber - 1);
            ImGui::Text("Enemies: %d", (int)m_Enemies.size());
            ImGui::Text("Points: %.0f", m_Player.Exp);
            if (!m_Player.ReloadTimer.IsDone())
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Reloading...");

            ImGui::EndChild();

            // Upgrade panel (center)
            if (m_ShowUpgradePanel)
            {
                ImVec2 avail = ImGui::GetContentRegionAvail();
                // avail is remaining from current cursor, so compute from viewport instead
                ImVec2 winPos = viewport->WorkPos;
                ImVec2 winSize = viewport->WorkSize;
                ImVec2 panelSize = ImVec2(600.0f, 480.0f);
                ImVec2 panelPos = ImVec2((winSize.x - panelSize.x) * 0.5f, (winSize.y - panelSize.y) * 0.5f);
                ImGui::SetCursorPos(panelPos);
                // Less transparent so it's readable over the game.
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.07f, 0.07f, 0.09f, 0.96f));
                ImGui::BeginChild("UpgradesPanel", panelSize, true);

                ImGui::SetWindowFontScale(1.10f * uiScale);

                ImGui::TextUnformatted("Upgrades");
                ImGui::Separator();
                ImGui::TextColored(ImVec4(0.0f, 0.8f, 0.4f, 1.0f), "Points: %.0f", m_Player.Exp);
                ImGui::Spacing();

                for (int i = 0; i < (int)m_Player.Upgrades.size(); i++)
                {
                    auto& upgrade = m_Player.Upgrades[i];
                    ImGui::PushID(i);

                    bool canAfford = upgrade.CanAfford(m_Player.Exp);
                    ImGuiDisabledScope disabledScope(!canAfford);

                    if (ImGui::Button(upgrade.Name.c_str(), ImVec2(140.0f, 30.0f)))
                        m_Player.ApplyUpgrade(i);

                    ImGui::SameLine();
                    ImGui::Text("Lv %d/%d", upgrade.Level, upgrade.MaxLevel);
                    ImGui::SameLine();
                    ImGui::TextColored(
                        canAfford ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
                        "Cost: %d", upgrade.GetCost());

                    ImGui::PopID();
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Press TAB to close");

                ImGui::EndChild();
                ImGui::PopStyleColor();

                // Restore base scale for other overlays.
                ImGui::SetWindowFontScale(uiScale);
            }

            // Death screen overlay (center)
            if (m_Player.State == SwarmSlayer::PlayerState::Dead)
            {
                ImVec2 winSize = viewport->WorkSize;
                ImVec2 panelSize = ImVec2(420.0f, 220.0f);
                ImVec2 panelPos = ImVec2((winSize.x - panelSize.x) * 0.5f, (winSize.y - panelSize.y) * 0.5f);
                ImGui::SetCursorPos(panelPos);
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.07f, 0.07f, 0.09f, 0.94f));
                ImGui::BeginChild("GameOverPanel", panelSize, true);

                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
                ImGui::SetWindowFontScale(1.7f * uiScale);
                ImGui::TextUnformatted("GAME OVER");
                ImGui::PopStyleColor();

                ImGui::SetWindowFontScale(1.05f * uiScale);
                ImGui::Text("You survived until Wave %d", m_WaveManager.WaveNumber - 1);
                ImGui::Text("Final Score: %.0f", m_Player.Exp);
                ImGui::Spacing();
                if (ImGui::Button("Restart", ImVec2(120.0f, 32.0f)))
                    RestartGame();

                ImGui::EndChild();
                ImGui::PopStyleColor();

                // Restore base scale.
                ImGui::SetWindowFontScale(uiScale);
            }
        }

        ImGui::End();
        ImGui::PopStyleColor(1);
        ImGui::PopStyleVar(3);
    }

private:
    bool CheckCollision(const glm::vec4& a, const glm::vec4& b) const
    {
        return a.x < b.x + b.z && a.x + a.z > b.x &&
               a.y < b.y + b.w && a.y + a.w > b.y;
    }

    glm::vec2 GetMouseWorldPosition() const
    {
        auto [mx, my] = Pillar::Input::GetMousePosition();
        auto& window = Pillar::Application::Get().GetWindow();
        float windowWidth = static_cast<float>(window.GetWidth());
        float windowHeight = static_cast<float>(window.GetHeight());

        // Convert screen coords to normalized device coords
        float ndcX = (2.0f * mx / windowWidth) - 1.0f;
        float ndcY = 1.0f - (2.0f * my / windowHeight);

        // Apply inverse camera transform
        const auto& camera = m_CameraController.GetCamera();
        glm::vec3 camPos = camera.GetPosition();
        float zoom = m_CameraController.GetZoomLevel();
        float aspectRatio = windowWidth / windowHeight;

        return {
            camPos.x + ndcX * zoom * aspectRatio,
            camPos.y + ndcY * zoom
        };
    }

    void RestartGame()
    {
        m_Player = SwarmSlayer::Player();
        m_Enemies.clear();
        m_Bullets.clear();
        m_ExpOrbs.clear();
        m_WaveManager = SwarmSlayer::WaveManager();
        m_GameState = SwarmSlayer::GameState::Playing;
        m_ShowUpgradePanel = false;
        m_WasMouseLeftDown = false;
        m_WasTabDown = false;
        m_MouseLeftPressedThisFrame = false;

        // Restart background music after death + restart.
        if (m_BackgroundMusic)
        {
            m_BackgroundMusic->SetLooping(true);
            m_BackgroundMusic->SetVolume(0.4f);
            m_BackgroundMusic->Play();
        }
    }

    void UpdatePolledInput()
    {
        // TAB toggle (edge triggered)
        bool tabDown = Pillar::Input::IsKeyPressed(PIL_KEY_TAB);
        if (tabDown && !m_WasTabDown)
        {
            m_ShowUpgradePanel = !m_ShowUpgradePanel;
        }
        m_WasTabDown = tabDown;

        // Mouse press (edge triggered)
        bool mouseDown = Pillar::Input::IsMouseButtonPressed(PIL_MOUSE_BUTTON_LEFT);
        m_MouseLeftPressedThisFrame = mouseDown && !m_WasMouseLeftDown;
        m_WasMouseLeftDown = mouseDown;

        // Menu start on click
        if (m_MouseLeftPressedThisFrame && m_GameState == SwarmSlayer::GameState::Menu)
        {
            m_GameState = SwarmSlayer::GameState::Playing;
            // Start background music
            if (m_BackgroundMusic)
                m_BackgroundMusic->Play();
        }
    }

private:
    SwarmSlayer::GameState m_GameState = SwarmSlayer::GameState::Menu;
    SwarmSlayer::Player m_Player;
    SwarmSlayer::WaveManager m_WaveManager;
    std::vector<SwarmSlayer::Enemy> m_Enemies;
    std::vector<SwarmSlayer::Bullet> m_Bullets;
    std::vector<SwarmSlayer::ExpOrb> m_ExpOrbs;
    std::vector<SwarmSlayer::Obstacle> m_Obstacles;
    bool m_ShowUpgradePanel = false;

    // Polled input edge detection
    bool m_WasMouseLeftDown = false;
    bool m_WasTabDown = false;
    bool m_MouseLeftPressedThisFrame = false;

    Pillar::OrthographicCameraController m_CameraController;

    // Textures
    std::shared_ptr<Pillar::Texture2D> m_PlayerTexture;
    std::shared_ptr<Pillar::Texture2D> m_GrassTexture;
    std::shared_ptr<Pillar::Texture2D> m_BoxTexture;
    std::shared_ptr<Pillar::Texture2D> m_BulletTexture;
    std::shared_ptr<Pillar::Texture2D> m_CoinTexture;
    std::shared_ptr<Pillar::Texture2D> m_GoblinTexture;
    std::shared_ptr<Pillar::Texture2D> m_OrbieTexture;
    std::shared_ptr<Pillar::Texture2D> m_HoodzyTexture;

    // Audio
    std::shared_ptr<Pillar::AudioClip> m_ShootSound;
    std::shared_ptr<Pillar::AudioClip> m_PlayerHurtSound;
    std::shared_ptr<Pillar::AudioClip> m_PlayerDiesSound;
    std::shared_ptr<Pillar::AudioClip> m_PickupCoinSound;
    std::shared_ptr<Pillar::AudioClip> m_HitEnemySound;
    std::shared_ptr<Pillar::AudioClip> m_BackgroundMusic;
};
