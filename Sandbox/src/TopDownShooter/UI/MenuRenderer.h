#pragma once

#include <imgui.h>
#include <Pillar/Logger.h>
#include <Pillar/Application.h>
#include <Pillar/Renderer/Texture.h>
#include <Pillar/Utils/AssetManager.h>
#include "../Core/GameState.h"
#include "../Components/PowerUpComponent.h"

namespace Game {

    // Floating damage number
    struct DamageNumber
    {
        glm::vec2 WorldPosition;
        int Damage;
        float Timer;        // Counts down
        float MaxTime;
    };

    // Notification banner
    struct WaveNotification
    {
        char Text[128] = "";
        char SubText[128] = "";
        float Timer = 0.0f;
        ImU32 Color = IM_COL32(255, 200, 50, 255);
    };

    /**
     * MenuRenderer - Renders all menu UIs (Main Menu, Pause, Game Over, Perk Selection)
     * Uses ImGui for dark/gritty SCRAPYARD SALVATION themed UI
     */
    class MenuRenderer
    {
    public:
        // Callbacks
        using StartGameCallback = std::function<void()>;
        using ResumeGameCallback = std::function<void()>;
        using RestartCallback = std::function<void()>;
        using QuitCallback = std::function<void()>;
        using PerkSelectedCallback = std::function<void(PerkType)>;
        using WeaponSelectedCallback = std::function<void(WeaponType)>;

        MenuRenderer()
        {
            // Load main menu background texture (SCRAPYARD SALVATION wasteland vista)
            m_MainMenuBackground = Pillar::Texture2D::Create(
                Pillar::AssetManager::GetTexturePath("UI/Main Menu/background.png"));
        }

        void SetCallbacks(
            StartGameCallback onStart,
            ResumeGameCallback onResume,
            RestartCallback onRestart,
            QuitCallback onQuit,
            PerkSelectedCallback onPerk,
            WeaponSelectedCallback onWeapon)
        {
            m_OnStartGame = onStart;
            m_OnResumeGame = onResume;
            m_OnRestart = onRestart;
            m_OnQuit = onQuit;
            m_OnPerkSelected = onPerk;
            m_OnWeaponSelected = onWeapon;
        }

        void Render()
        {
            ApplyDarkTheme();

            switch (GameState::Instance().GetState())
            {
                case GameStateType::MainMenu:
                    RenderMainMenu();
                    break;
                case GameStateType::Paused:
                    RenderPauseMenu();
                    break;
                case GameStateType::PerkSelection:
                    RenderPerkSelection();
                    break;
                case GameStateType::GameOver:
                    RenderGameOver();
                    break;
                default:
                    break;
            }
        }

        void RenderHUD()
        {
            if (GameState::Instance().GetState() != GameStateType::Playing)
                return;

            auto& stats = GameState::Instance().GetStats();
            auto& playerStats = GameState::Instance().GetPlayerStats();
            ImVec2 displaySize = ImGui::GetIO().DisplaySize;

            // ==== SINGLE CONSOLIDATED HUD OVERLAY ====
            // We use a single transparent fullscreen window as the HUD container
            ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
            ImGui::SetNextWindowSize(displaySize);
            ImGui::SetNextWindowBgAlpha(0.0f);  // Fully transparent background
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            
            ImGui::Begin("##GameHUD", nullptr,
                ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoScrollWithMouse |
                ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoNav |
                ImGuiWindowFlags_NoBackground |
                ImGuiWindowFlags_NoBringToFrontOnFocus |
                ImGuiWindowFlags_NoDocking);

            ImDrawList* drawList = ImGui::GetWindowDrawList();
            
            // ---- TOP-LEFT: Health, XP, Level (SCRAPYARD SALVATION industrial style) ----
            {
                float panelX = 15.0f;
                float panelY = 15.0f;
                float barWidth = 180.0f;
                float barHeight = 18.0f;
                
                // Health bar background - industrial dark gray with rust tint
                drawList->AddRectFilled(
                    ImVec2(panelX, panelY),
                    ImVec2(panelX + barWidth, panelY + barHeight),
                    IM_COL32(25, 20, 18, 220), 2.0f);
                // Industrial border
                drawList->AddRect(
                    ImVec2(panelX, panelY),
                    ImVec2(panelX + barWidth, panelY + barHeight),
                    IM_COL32(60, 50, 40, 200), 2.0f, 0, 1.5f);
                
                // Health bar fill - dried crimson to toxic green based on health
                float healthFill = barWidth * m_PlayerHealthPercent;
                if (healthFill > 0)
                {
                    // Low health: dried blood red, High health: toxic green (medstim color)
                    ImU32 healthColor = m_PlayerHealthPercent > 0.5f ? 
                        IM_COL32(85, 140, 60, 255) :    // Toxic green (healthy)
                        (m_PlayerHealthPercent > 0.25f ? 
                            IM_COL32(180, 80, 40, 255) :  // Oxidized orange (warning)
                            IM_COL32(140, 30, 25, 255));  // Dried crimson (critical)
                    drawList->AddRectFilled(
                        ImVec2(panelX, panelY),
                        ImVec2(panelX + healthFill, panelY + barHeight),
                        healthColor, 2.0f);
                }
                
                // Health text - harsh white with slight yellow tint
                char healthText[32];
                snprintf(healthText, sizeof(healthText), "HP: %.0f/%.0f", m_PlayerCurrentHealth, m_PlayerMaxHealth);
                drawList->AddText(ImVec2(panelX + 5, panelY + 2), IM_COL32(230, 225, 200, 255), healthText);
                
                // XP bar (smaller, below health) - industrial welding blue
                float xpY = panelY + barHeight + 5;
                float xpBarHeight = 12.0f;
                float xpPercent = static_cast<float>(stats.CurrentXP) / stats.XPToNextLevel;
                
                // XP background - dark industrial
                drawList->AddRectFilled(
                    ImVec2(panelX, xpY),
                    ImVec2(panelX + barWidth, xpY + xpBarHeight),
                    IM_COL32(18, 22, 28, 220), 2.0f);
                drawList->AddRect(
                    ImVec2(panelX, xpY),
                    ImVec2(panelX + barWidth, xpY + xpBarHeight),
                    IM_COL32(40, 50, 60, 180), 2.0f, 0, 1.0f);
                    
                if (xpPercent > 0)
                {
                    // Welding arc blue for XP
                    drawList->AddRectFilled(
                        ImVec2(panelX, xpY),
                        ImVec2(panelX + barWidth * xpPercent, xpY + xpBarHeight),
                        IM_COL32(60, 140, 180, 255), 2.0f);
                }
                
                char xpText[32];
                snprintf(xpText, sizeof(xpText), "LV %d - %d/%d XP", stats.PlayerLevel, stats.CurrentXP, stats.XPToNextLevel);
                drawList->AddText(ImVec2(panelX + 5, xpY), IM_COL32(140, 180, 200, 220), xpText);
            }
            
            // ---- TOP-CENTER: Wave Indicator (SCRAPYARD SALVATION - industrial military style) ----
            {
                char waveText[32];
                snprintf(waveText, sizeof(waveText), "WAVE %d", stats.WaveReached);
                ImVec2 textSize = ImGui::CalcTextSize(waveText);
                float waveX = (displaySize.x - textSize.x * 1.5f) * 0.5f;
                
                // Industrial background - rusted metal plate look
                drawList->AddRectFilled(
                    ImVec2(waveX - 15, 10),
                    ImVec2(waveX + textSize.x * 1.5f + 15, 40),
                    IM_COL32(35, 28, 22, 220), 3.0f);
                // Hazard border
                drawList->AddRect(
                    ImVec2(waveX - 15, 10),
                    ImVec2(waveX + textSize.x * 1.5f + 15, 40),
                    IM_COL32(180, 120, 40, 200), 3.0f, 0, 2.0f);
                    
                // Hazard yellow-orange text
                drawList->AddText(ImGui::GetFont(), ImGui::GetFontSize() * 1.5f,
                    ImVec2(waveX, 12), IM_COL32(220, 160, 40, 255), waveText);
            }
            
            // ---- TOP-RIGHT: Score & Kills (SCRAPYARD SALVATION style) ----
            {
                char scoreText[32];
                snprintf(scoreText, sizeof(scoreText), "SCORE: %d", stats.Score);
                char killsText[32];
                snprintf(killsText, sizeof(killsText), "KILLS: %d", stats.TotalKills);
                
                ImVec2 scoreSize = ImGui::CalcTextSize(scoreText);
                ImVec2 killsSize = ImGui::CalcTextSize(killsText);
                float maxWidth = std::max(scoreSize.x, killsSize.x);
                
                float panelX = displaySize.x - maxWidth - 25;
                float panelY = 15;
                
                // Industrial dark background with rust tint
                drawList->AddRectFilled(
                    ImVec2(panelX - 10, panelY - 5),
                    ImVec2(displaySize.x - 10, panelY + 40),
                    IM_COL32(25, 20, 18, 200), 3.0f);
                drawList->AddRect(
                    ImVec2(panelX - 10, panelY - 5),
                    ImVec2(displaySize.x - 10, panelY + 40),
                    IM_COL32(60, 50, 40, 180), 3.0f, 0, 1.0f);
                
                // Score: hazard yellow, Kills: dried crimson
                drawList->AddText(ImVec2(panelX, panelY), IM_COL32(220, 180, 50, 255), scoreText);
                drawList->AddText(ImVec2(panelX, panelY + 20), IM_COL32(160, 50, 40, 255), killsText);
            }
            
            // ---- BOTTOM-LEFT: Dash Indicator (SCRAPYARD SALVATION style) ----
            {
                float panelX = 15;
                float panelY = displaySize.y - 45;
                
                const char* dashText = m_DashReady ? "[DASH READY]" : "";
                char dashCooldownText[32] = "";
                if (!m_DashReady)
                    snprintf(dashCooldownText, sizeof(dashCooldownText), "[DASH: %.1fs]", m_DashCooldown);
                
                const char* displayText = m_DashReady ? dashText : dashCooldownText;
                // Welding blue when ready, rusty gray when on cooldown
                ImU32 dashColor = m_DashReady ? IM_COL32(60, 160, 200, 255) : IM_COL32(90, 80, 70, 200);
                
                drawList->AddText(ImVec2(panelX, panelY), dashColor, displayText);
            }
            
            // ---- BOTTOM-CENTER: Weapon & Controls (SCRAPYARD SALVATION style) ----
            {
                auto currentWeapon = GameState::Instance().GetCurrentWeapon();
                auto& weapon = GameState::Instance().GetWeapon(currentWeapon);
                
                char weaponText[128];
                snprintf(weaponText, sizeof(weaponText), "[%s]  WASD:Move  LMB:Shoot  Space:Dash  1-5:Weapons", 
                    weapon.Name.c_str());
                
                ImVec2 textSize = ImGui::CalcTextSize(weaponText);
                float textX = (displaySize.x - textSize.x) * 0.5f;
                float textY = displaySize.y - 30;
                
                // Industrial dark background
                drawList->AddRectFilled(
                    ImVec2(textX - 10, textY - 5),
                    ImVec2(textX + textSize.x + 10, textY + textSize.y + 5),
                    IM_COL32(20, 18, 16, 200), 3.0f);
                
                // Muted toxic green for weapon info
                drawList->AddText(ImVec2(textX, textY), IM_COL32(120, 170, 100, 230), weaponText);
            }
            
            // ---- Kill Streak Notification (SCRAPYARD - hazard orange) ----
            if (m_KillStreakTimer > 0.0f)
            {
                float alpha = std::min(1.0f, m_KillStreakTimer * 2.0f);
                float scale = 1.5f + (1.0f - alpha) * 0.5f;
                
                ImVec2 textSize = ImGui::CalcTextSize(m_KillStreakText);
                float textX = (displaySize.x - textSize.x * scale) * 0.5f;
                float textY = displaySize.y * 0.3f;
                
                // Hazard orange kill streak
                ImU32 streakColor = IM_COL32(220, 140, 30, static_cast<int>(alpha * 255));
                drawList->AddText(ImGui::GetFont(), ImGui::GetFontSize() * scale,
                    ImVec2(textX, textY), streakColor, m_KillStreakText);
            }
            
            // ---- Level Up Notification (SCRAPYARD - welding arc blue) ----
            if (m_LevelUpTimer > 0.0f)
            {
                float alpha = std::min(1.0f, m_LevelUpTimer);
                float bounce = std::sin(m_LevelUpTimer * 10.0f) * 5.0f;

                const char* levelUpText = "LEVEL UP!";
                ImVec2 textSize = ImGui::CalcTextSize(levelUpText);
                float textX = (displaySize.x - textSize.x * 2.0f) * 0.5f;
                float textY = displaySize.y * 0.4f + bounce;

                // Welding arc blue for level up
                ImU32 levelColor = IM_COL32(80, 180, 220, static_cast<int>(alpha * 255));
                drawList->AddText(ImGui::GetFont(), ImGui::GetFontSize() * 2.0f,
                    ImVec2(textX, textY), levelColor, levelUpText);
            }

            // ---- Enemies Remaining (below wave indicator) ----
            if (m_EnemiesRemaining > 0)
            {
                char enemyText[32];
                snprintf(enemyText, sizeof(enemyText), "%d enemies remaining", m_EnemiesRemaining);
                ImVec2 textSize = ImGui::CalcTextSize(enemyText);
                float textX = (displaySize.x - textSize.x) * 0.5f;
                drawList->AddText(ImVec2(textX, 45), IM_COL32(200, 200, 200, 180), enemyText);
            }

            // ---- Wave Countdown (between waves) ----
            if (m_WaveCountdown > 0.1f)
            {
                char countdownText[64];
                snprintf(countdownText, sizeof(countdownText), "NEXT WAVE IN %.1fs", m_WaveCountdown);
                ImVec2 textSize = ImGui::CalcTextSize(countdownText);
                float textX = (displaySize.x - textSize.x * 1.3f) * 0.5f;
                float textY = displaySize.y * 0.5f;
                drawList->AddText(ImGui::GetFont(), ImGui::GetFontSize() * 1.3f,
                    ImVec2(textX, textY), IM_COL32(200, 200, 100, 200), countdownText);
            }

            // ---- Wave Start/Complete Notification ----
            if (m_WaveNotification.Timer > 0.0f)
            {
                float alpha = std::min(1.0f, m_WaveNotification.Timer);
                float scale = 2.0f;

                ImVec2 textSize = ImGui::CalcTextSize(m_WaveNotification.Text);
                float textX = (displaySize.x - textSize.x * scale) * 0.5f;
                float textY = displaySize.y * 0.35f;

                ImU32 color = m_WaveNotification.Color;
                // Apply alpha
                int a = static_cast<int>(alpha * 255);
                color = (color & 0x00FFFFFF) | (static_cast<ImU32>(a) << 24);

                drawList->AddText(ImGui::GetFont(), ImGui::GetFontSize() * scale,
                    ImVec2(textX, textY), color, m_WaveNotification.Text);

                // Sub-text (bonus info)
                if (m_WaveNotification.SubText[0] != '\0')
                {
                    ImVec2 subSize = ImGui::CalcTextSize(m_WaveNotification.SubText);
                    float subX = (displaySize.x - subSize.x * 1.2f) * 0.5f;
                    ImU32 subColor = IM_COL32(255, 255, 100, a);
                    drawList->AddText(ImGui::GetFont(), ImGui::GetFontSize() * 1.2f,
                        ImVec2(subX, textY + ImGui::GetFontSize() * scale + 5), subColor, m_WaveNotification.SubText);
                }
            }

            // ---- Combo Counter ----
            if (m_ComboTimer > 0.0f)
            {
                float alpha = std::min(1.0f, m_ComboTimer);
                ImVec2 textSize = ImGui::CalcTextSize(m_ComboText);
                float textX = (displaySize.x - textSize.x * 1.4f) * 0.5f;
                float textY = displaySize.y * 0.25f;

                ImU32 comboColor = IM_COL32(255, 150, 50, static_cast<int>(alpha * 255));
                drawList->AddText(ImGui::GetFont(), ImGui::GetFontSize() * 1.4f,
                    ImVec2(textX, textY), comboColor, m_ComboText);
            }

            // ---- Active Buffs Display (below dash indicator, bottom-left) ----
            if (!m_ActiveBuffs.empty())
            {
                float buffX = 15.0f;
                float buffY = displaySize.y - 70.0f;

                for (const auto& buff : m_ActiveBuffs)
                {
                    const char* buffName = "";
                    ImU32 buffColor = IM_COL32(255, 255, 255, 220);

                    switch (buff.Type)
                    {
                        case PowerUpType::SpeedBoost:
                            buffName = "SPEED";
                            buffColor = IM_COL32(255, 255, 50, 220);
                            break;
                        case PowerUpType::FireRateUp:
                            buffName = "FIRE RATE";
                            buffColor = IM_COL32(255, 128, 0, 220);
                            break;
                        case PowerUpType::DamageUp:
                            buffName = "DAMAGE";
                            buffColor = IM_COL32(255, 50, 50, 220);
                            break;
                        case PowerUpType::Shield:
                            buffName = "SHIELD";
                            buffColor = IM_COL32(50, 128, 255, 220);
                            break;
                        case PowerUpType::Magnet:
                            buffName = "MAGNET";
                            buffColor = IM_COL32(255, 50, 255, 220);
                            break;
                        default:
                            continue;
                    }

                    char buffText[64];
                    snprintf(buffText, sizeof(buffText), "%s %.1fs", buffName, buff.RemainingDuration);

                    // Background bar
                    ImVec2 tSize = ImGui::CalcTextSize(buffText);
                    float barWidth = tSize.x + 10.0f;
                    float barHeight = tSize.y + 4.0f;
                    drawList->AddRectFilled(
                        ImVec2(buffX, buffY),
                        ImVec2(buffX + barWidth, buffY + barHeight),
                        IM_COL32(20, 20, 20, 160), 3.0f);

                    // Remaining duration fill
                    float fillPercent = buff.RemainingDuration / 10.0f;  // Approximate max
                    fillPercent = std::min(1.0f, fillPercent);
                    ImU32 fillColor = (buffColor & 0x00FFFFFF) | IM_COL32(0, 0, 0, 60);
                    drawList->AddRectFilled(
                        ImVec2(buffX, buffY),
                        ImVec2(buffX + barWidth * fillPercent, buffY + barHeight),
                        fillColor, 3.0f);

                    drawList->AddText(ImVec2(buffX + 5, buffY + 2), buffColor, buffText);

                    buffY -= barHeight + 3.0f;
                }
            }

            // ---- Damage Numbers ----
            // Note: These are world-space positions rendered via screen projection
            // For simplicity we render them at fixed screen offset from center
            // A proper implementation would project world->screen, but this gives reasonable results
            for (const auto& dn : m_DamageNumbers)
            {
                float alpha = dn.Timer / dn.MaxTime;
                float scale = 1.0f + (1.0f - alpha) * 0.3f;

                char dmgText[16];
                snprintf(dmgText, sizeof(dmgText), "%d", dn.Damage);

                // Approximate screen position from world offset
                // World center is at screen center; scale by ~40 pixels per world unit
                float screenX = displaySize.x * 0.5f + dn.WorldPosition.x * 40.0f;
                float screenY = displaySize.y * 0.5f - dn.WorldPosition.y * 40.0f;

                // Clamp to screen
                screenX = std::max(10.0f, std::min(displaySize.x - 50.0f, screenX));
                screenY = std::max(10.0f, std::min(displaySize.y - 30.0f, screenY));

                ImU32 dmgColor = dn.Damage >= 20
                    ? IM_COL32(255, 80, 80, static_cast<int>(alpha * 255))
                    : IM_COL32(255, 220, 100, static_cast<int>(alpha * 255));

                drawList->AddText(ImGui::GetFont(), ImGui::GetFontSize() * scale,
                    ImVec2(screenX, screenY), dmgColor, dmgText);
            }

            ImGui::End();
            ImGui::PopStyleVar(2);
        }
        
        // Call this when player gets a kill streak
        void TriggerKillStreak(int kills)
        {
            if (kills >= 10)
            {
                snprintf(m_KillStreakText, sizeof(m_KillStreakText), "UNSTOPPABLE! x%d", kills);
                m_KillStreakTimer = 2.0f;
            }
            else if (kills >= 5)
            {
                snprintf(m_KillStreakText, sizeof(m_KillStreakText), "KILLING SPREE! x%d", kills);
                m_KillStreakTimer = 1.5f;
            }
            else if (kills >= 3)
            {
                snprintf(m_KillStreakText, sizeof(m_KillStreakText), "TRIPLE KILL!");
                m_KillStreakTimer = 1.0f;
            }
        }
        
        void TriggerLevelUp()
        {
            m_LevelUpTimer = 2.0f;
        }
        
        void UpdateTimers(float dt)
        {
            if (m_KillStreakTimer > 0.0f) m_KillStreakTimer -= dt;
            if (m_LevelUpTimer > 0.0f) m_LevelUpTimer -= dt;
            if (m_WaveNotification.Timer > 0.0f) m_WaveNotification.Timer -= dt;
            if (m_ComboTimer > 0.0f) m_ComboTimer -= dt;

            // Update damage numbers
            for (auto& dn : m_DamageNumbers)
            {
                dn.Timer -= dt;
                dn.WorldPosition.y += dt * 2.0f;  // Float upward
            }
            m_DamageNumbers.erase(
                std::remove_if(m_DamageNumbers.begin(), m_DamageNumbers.end(),
                    [](const DamageNumber& dn) { return dn.Timer <= 0.0f; }),
                m_DamageNumbers.end()
            );
        }

        void SetPlayerHealth(float current, float max)
        {
            m_PlayerHealthPercent = current / max;
            m_PlayerCurrentHealth = current;
            m_PlayerMaxHealth = max;
        }
        
        void SetDashCooldown(float cooldown, bool ready)
        {
            m_DashCooldown = cooldown;
            m_DashReady = ready;
        }

        void SetWaveCountdown(float seconds) { m_WaveCountdown = seconds; }
        void SetEnemiesRemaining(int count) { m_EnemiesRemaining = count; }

        void SetActiveBuffs(const std::vector<ActivePowerUpEffect>& buffs)
        {
            m_ActiveBuffs = buffs;
        }

        void ClearActiveBuffs()
        {
            m_ActiveBuffs.clear();
        }

        void AddDamageNumber(const glm::vec2& worldPos, int damage)
        {
            if (m_DamageNumbers.size() < 30)  // Cap to avoid overflow
            {
                m_DamageNumbers.push_back({ worldPos, damage, 0.8f, 0.8f });
            }
        }

        void TriggerWaveStart(int waveNumber)
        {
            snprintf(m_WaveNotification.Text, sizeof(m_WaveNotification.Text), "WAVE %d", waveNumber);
            m_WaveNotification.SubText[0] = '\0';
            m_WaveNotification.Timer = 2.0f;
            m_WaveNotification.Color = IM_COL32(255, 200, 50, 255);
        }

        void TriggerWaveComplete(int waveNumber, int bonus, bool noDamage)
        {
            snprintf(m_WaveNotification.Text, sizeof(m_WaveNotification.Text), "WAVE %d COMPLETE!", waveNumber);
            if (noDamage)
                snprintf(m_WaveNotification.SubText, sizeof(m_WaveNotification.SubText), "+%d (PERFECT WAVE!)", bonus);
            else
                snprintf(m_WaveNotification.SubText, sizeof(m_WaveNotification.SubText), "+%d BONUS", bonus);
            m_WaveNotification.Timer = 2.5f;
            m_WaveNotification.Color = noDamage ? IM_COL32(100, 255, 100, 255) : IM_COL32(255, 200, 50, 255);
        }

        void TriggerCombo(int comboCount, int scoreGained)
        {
            snprintf(m_ComboText, sizeof(m_ComboText), "COMBO x%d  +%d", comboCount, scoreGained);
            m_ComboTimer = 1.5f;
        }

    private:
        // SCRAPYARD SALVATION - Industrial Gritty Theme
        void ApplyDarkTheme()
        {
            ImGuiStyle& style = ImGui::GetStyle();
            style.WindowRounding = 2.0f;   // Sharper, more industrial
            style.FrameRounding = 1.0f;
            style.WindowBorderSize = 1.5f;
            style.FrameBorderSize = 1.0f;
            
            // Charred black background with slight warmth
            style.Colors[ImGuiCol_WindowBg] = ImVec4(0.04f, 0.035f, 0.03f, 0.97f);
            
            // Buttons: Rusted metal with hazard orange accents
            style.Colors[ImGuiCol_Button] = ImVec4(0.18f, 0.12f, 0.08f, 1.0f);
            style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.35f, 0.18f, 0.08f, 1.0f);
            style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.55f, 0.25f, 0.10f, 1.0f);
            
            // Title bars: Dark industrial steel
            style.Colors[ImGuiCol_TitleBg] = ImVec4(0.06f, 0.05f, 0.04f, 1.0f);
            style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.12f, 0.08f, 0.06f, 1.0f);
            
            // Frames: Rusty brown tint
            style.Colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.08f, 0.06f, 1.0f);
            style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.18f, 0.12f, 0.08f, 1.0f);
            
            // Headers: Industrial orange-brown
            style.Colors[ImGuiCol_Header] = ImVec4(0.22f, 0.14f, 0.08f, 1.0f);
            style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.35f, 0.20f, 0.10f, 1.0f);
            style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.45f, 0.25f, 0.12f, 1.0f);
            
            // Borders: Rusty metal
            style.Colors[ImGuiCol_Border] = ImVec4(0.25f, 0.18f, 0.12f, 0.6f);
            
            // Text: Slightly yellowed harsh white
            style.Colors[ImGuiCol_Text] = ImVec4(0.90f, 0.88f, 0.80f, 1.0f);
            style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.45f, 0.42f, 0.38f, 1.0f);
            
            // Separator: Industrial brown
            style.Colors[ImGuiCol_Separator] = ImVec4(0.30f, 0.22f, 0.15f, 0.8f);
        }

        void RenderMainMenu()
        {
            ImVec2 displaySize = ImGui::GetIO().DisplaySize;
            
            // Full screen background with SCRAPYARD SALVATION wasteland vista texture
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(displaySize);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            ImGui::Begin("##MainMenuBG", nullptr,
                ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoInputs |
                ImGuiWindowFlags_NoDocking);
            
            // Render the background texture if available
            if (m_MainMenuBackground)
            {
                ImTextureID texId = (ImTextureID)(size_t)m_MainMenuBackground->GetRendererID();
                ImGui::Image(texId, displaySize, ImVec2(0, 1), ImVec2(1, 0));  // Flip UV for OpenGL
            }
            ImGui::End();
            ImGui::PopStyleVar();

            // Center window - semi-transparent to let background show through
            ImGui::SetNextWindowPos(ImVec2(displaySize.x * 0.5f, displaySize.y * 0.5f), 
                ImGuiCond_Always, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(400, 500));
            ImGui::SetNextWindowBgAlpha(0.85f);  // Let background texture show through edges
            
            ImGui::Begin("##MainMenu", nullptr,
                ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoDocking);

            // Title - SCRAPYARD SALVATION
            ImGui::SetCursorPosX((400 - ImGui::CalcTextSize("SCRAPYARD").x * 2.2f) * 0.5f);
            ImGui::SetWindowFontScale(2.2f);
            ImGui::TextColored(ImVec4(0.70f, 0.45f, 0.15f, 1.0f), "SCRAPYARD");  // Oxidized orange
            ImGui::SetWindowFontScale(1.0f);
            
            ImGui::SetCursorPosX((400 - ImGui::CalcTextSize("SALVATION").x * 2.2f) * 0.5f);
            ImGui::SetWindowFontScale(2.2f);
            ImGui::TextColored(ImVec4(0.55f, 0.12f, 0.10f, 1.0f), "SALVATION");  // Dried crimson
            ImGui::SetWindowFontScale(1.0f);

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // Tagline
            ImGui::SetCursorPosX((400 - ImGui::CalcTextSize("Rust never sleeps. Neither do the horrors.").x) * 0.5f);
            ImGui::TextColored(ImVec4(0.50f, 0.45f, 0.38f, 1.0f), "Rust never sleeps. Neither do the horrors.");

            ImGui::Spacing();
            ImGui::Spacing();
            ImGui::Spacing();

            // Buttons
            float buttonWidth = 200.0f;
            float buttonHeight = 50.0f;
            ImGui::SetCursorPosX((400 - buttonWidth) * 0.5f);

            if (ImGui::Button("PLAY", ImVec2(buttonWidth, buttonHeight)))
            {
                if (m_OnStartGame) m_OnStartGame();
            }

            ImGui::Spacing();
            ImGui::SetCursorPosX((400 - buttonWidth) * 0.5f);
            if (ImGui::Button("HIGH SCORES", ImVec2(buttonWidth, buttonHeight)))
            {
                m_ShowHighScores = !m_ShowHighScores;
            }

            ImGui::Spacing();
            ImGui::SetCursorPosX((400 - buttonWidth) * 0.5f);
            if (ImGui::Button("QUIT", ImVec2(buttonWidth, buttonHeight)))
            {
                if (m_OnQuit) m_OnQuit();
            }

            // High scores panel
            if (m_ShowHighScores)
            {
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "HIGH SCORES");
                ImGui::Separator();

                const auto& scores = GameState::Instance().GetHighScores();
                if (scores.empty())
                {
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No scores yet!");
                }
                else
                {
                    for (size_t i = 0; i < scores.size() && i < 5; ++i)
                    {
                        ImGui::Text("%zu. %s - %d (Wave %d)", 
                            i + 1, scores[i].Name.c_str(), scores[i].Score, scores[i].Wave);
                    }
                }
            }

            // Controls
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "CONTROLS:");
            ImGui::BulletText("WASD - Move");
            ImGui::BulletText("Mouse - Aim");
            ImGui::BulletText("Left Click - Shoot");
            ImGui::BulletText("Space - Dash");
            ImGui::BulletText("1-5 - Switch Weapons");
            ImGui::BulletText("ESC - Pause");

            ImGui::End();
        }

        void RenderPauseMenu()
        {
            ImVec2 displaySize = ImGui::GetIO().DisplaySize;

            // Dim background
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(displaySize);
            ImGui::SetNextWindowBgAlpha(0.5f);
            ImGui::Begin("##PauseBG", nullptr,
                ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoInputs |
                ImGuiWindowFlags_NoDocking);
            ImGui::End();

            // Pause window
            ImGui::SetNextWindowPos(ImVec2(displaySize.x * 0.5f, displaySize.y * 0.5f), 
                ImGuiCond_Always, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(300, 250));

            ImGui::Begin("##Pause", nullptr,
                ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoDocking);

            ImGui::SetCursorPosX((300 - ImGui::CalcTextSize("PAUSED").x * 2.0f) * 0.5f);
            ImGui::SetWindowFontScale(2.0f);
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "PAUSED");
            ImGui::SetWindowFontScale(1.0f);

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            float buttonWidth = 180.0f;
            float buttonHeight = 40.0f;

            ImGui::SetCursorPosX((300 - buttonWidth) * 0.5f);
            if (ImGui::Button("RESUME", ImVec2(buttonWidth, buttonHeight)))
            {
                if (m_OnResumeGame) m_OnResumeGame();
            }

            ImGui::Spacing();
            ImGui::SetCursorPosX((300 - buttonWidth) * 0.5f);
            if (ImGui::Button("RESTART", ImVec2(buttonWidth, buttonHeight)))
            {
                if (m_OnRestart) m_OnRestart();
            }

            ImGui::Spacing();
            ImGui::SetCursorPosX((300 - buttonWidth) * 0.5f);
            if (ImGui::Button("QUIT TO MENU", ImVec2(buttonWidth, buttonHeight)))
            {
                GameState::Instance().SetState(GameStateType::MainMenu);
            }

            ImGui::End();
        }

        void RenderPerkSelection()
        {
            ImVec2 displaySize = ImGui::GetIO().DisplaySize;

            // Dim background
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(displaySize);
            ImGui::SetNextWindowBgAlpha(0.7f);
            ImGui::Begin("##PerkBG", nullptr,
                ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoInputs |
                ImGuiWindowFlags_NoDocking);
            ImGui::End();

            // Get random perks if not already generated
            if (m_CurrentPerkChoices.empty())
            {
                m_CurrentPerkChoices = GameState::Instance().GetRandomPerks(3);
            }

            // Perk selection window
            ImGui::SetNextWindowPos(ImVec2(displaySize.x * 0.5f, displaySize.y * 0.5f), 
                ImGuiCond_Always, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(600, 400));

            ImGui::Begin("##PerkSelect", nullptr,
                ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoDocking);

            ImGui::SetCursorPosX((600 - ImGui::CalcTextSize("CHOOSE A PERK").x * 1.8f) * 0.5f);
            ImGui::SetWindowFontScale(1.8f);
            ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.4f, 1.0f), "CHOOSE A PERK");
            ImGui::SetWindowFontScale(1.0f);

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // Display perk choices
            float perkWidth = 170.0f;
            float perkHeight = 200.0f;
            float totalWidth = perkWidth * m_CurrentPerkChoices.size() + 20.0f * (m_CurrentPerkChoices.size() - 1);
            float startX = (600 - totalWidth) * 0.5f;

            for (size_t i = 0; i < m_CurrentPerkChoices.size(); ++i)
            {
                PerkInfo* perk = m_CurrentPerkChoices[i];
                
                ImGui::SetCursorPos(ImVec2(startX + i * (perkWidth + 20.0f), 80));
                
                ImGui::BeginChild(("##Perk" + std::to_string(i)).c_str(), 
                    ImVec2(perkWidth, perkHeight), true);

                // Perk name
                ImGui::SetWindowFontScale(1.2f);
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "%s", perk->Name.c_str());
                ImGui::SetWindowFontScale(1.0f);

                // Level indicator
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), 
                    "Lv %d / %d", perk->CurrentLevel + 1, perk->MaxLevel);

                ImGui::Separator();

                // Description
                ImGui::TextWrapped("%s", perk->Description.c_str());

                ImGui::SetCursorPosY(perkHeight - 40);
                if (ImGui::Button("SELECT", ImVec2(perkWidth - 16, 30)))
                {
                    if (m_OnPerkSelected)
                    {
                        m_OnPerkSelected(perk->Type);
                        m_CurrentPerkChoices.clear();  // Reset for next selection
                    }
                }

                ImGui::EndChild();
            }

            // Also show weapon unlocks if available
            ImGui::SetCursorPosY(300);
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "AVAILABLE WEAPON UNLOCKS:");

            const auto& weapons = GameState::Instance().GetWeapons();
            for (const auto& weapon : weapons)
            {
                if (!weapon.Unlocked && GameState::Instance().CanUnlockWeapon(weapon.Type))
                {
                    ImGui::SameLine();
                    std::string label = weapon.Name + " (Req: Lv" + std::to_string(weapon.XPCostToUnlock) + ")";
                    if (ImGui::Button(label.c_str()))
                    {
                        if (m_OnWeaponSelected)
                            m_OnWeaponSelected(weapon.Type);
                    }
                }
            }

            ImGui::End();
        }

        void RenderGameOver()
        {
            ImVec2 displaySize = ImGui::GetIO().DisplaySize;
            auto& stats = GameState::Instance().GetStats();

            // Full screen dark overlay - industrial blackout
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(displaySize);
            ImGui::SetNextWindowBgAlpha(0.85f);
            ImGui::Begin("##GameOverBG", nullptr,
                ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoInputs |
                ImGuiWindowFlags_NoDocking);
            ImGui::End();

            // Game over window
            ImGui::SetNextWindowPos(ImVec2(displaySize.x * 0.5f, displaySize.y * 0.5f), 
                ImGuiCond_Always, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(450, 400));

            ImGui::Begin("##GameOver", nullptr,
                ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoDocking);

            // Title - SCRAPYARD style
            ImGui::SetCursorPosX((450 - ImGui::CalcTextSize("SALVAGE FAILED").x * 2.2f) * 0.5f);
            ImGui::SetWindowFontScale(2.2f);
            ImGui::TextColored(ImVec4(0.55f, 0.12f, 0.10f, 1.0f), "SALVAGE FAILED");  // Dried crimson
            ImGui::SetWindowFontScale(1.0f);

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // Stats - hazard yellow/orange theme
            ImGui::SetWindowFontScale(1.3f);
            ImGui::TextColored(ImVec4(0.85f, 0.65f, 0.20f, 1.0f), "FINAL SCORE: %d", stats.Score);
            ImGui::SetWindowFontScale(1.0f);

            ImGui::Spacing();
            // Stats in muted industrial colors
            ImGui::TextColored(ImVec4(0.75f, 0.70f, 0.60f, 1.0f), "Wave Reached: %d", stats.WaveReached);
            ImGui::TextColored(ImVec4(0.75f, 0.70f, 0.60f, 1.0f), "Total Kills: %d", stats.TotalKills);
            ImGui::TextColored(ImVec4(0.75f, 0.70f, 0.60f, 1.0f), "Bosses Killed: %d", stats.BossesKilled);
            ImGui::TextColored(ImVec4(0.75f, 0.70f, 0.60f, 1.0f), "Player Level: %d", stats.PlayerLevel);
            
            int minutes = static_cast<int>(stats.PlayTime) / 60;
            int seconds = static_cast<int>(stats.PlayTime) % 60;
            ImGui::TextColored(ImVec4(0.75f, 0.70f, 0.60f, 1.0f), "Survival Time: %02d:%02d", minutes, seconds);

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // High score entry - welding blue accent
            if (GameState::Instance().IsHighScore(stats.Score))
            {
                ImGui::TextColored(ImVec4(0.30f, 0.60f, 0.75f, 1.0f), "NEW HIGH SCORE!");
                
                static char nameBuffer[32] = "Salvager";
                ImGui::SetNextItemWidth(200);
                ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer));
                
                ImGui::SameLine();
                if (ImGui::Button("Save Score"))
                {
                    GameState::Instance().AddHighScore(nameBuffer, stats.Score, stats.WaveReached, stats.TotalKills);
                }
            }

            ImGui::Spacing();
            ImGui::Spacing();

            // Buttons
            float buttonWidth = 180.0f;
            float buttonHeight = 45.0f;

            ImGui::SetCursorPosX((450 - buttonWidth) * 0.5f);
            if (ImGui::Button("PLAY AGAIN", ImVec2(buttonWidth, buttonHeight)))
            {
                if (m_OnRestart) m_OnRestart();
            }

            ImGui::Spacing();
            ImGui::SetCursorPosX((450 - buttonWidth) * 0.5f);
            if (ImGui::Button("MAIN MENU", ImVec2(buttonWidth, buttonHeight)))
            {
                GameState::Instance().Reset();
                GameState::Instance().SetState(GameStateType::MainMenu);
            }

            ImGui::End();
        }

    private:
        StartGameCallback m_OnStartGame;
        ResumeGameCallback m_OnResumeGame;
        RestartCallback m_OnRestart;
        QuitCallback m_OnQuit;
        PerkSelectedCallback m_OnPerkSelected;
        WeaponSelectedCallback m_OnWeaponSelected;

        std::vector<PerkInfo*> m_CurrentPerkChoices;
        bool m_ShowHighScores = false;

        // UI Textures (SCRAPYARD SALVATION themed)
        std::shared_ptr<Pillar::Texture2D> m_MainMenuBackground;

        // HUD state
        float m_PlayerHealthPercent = 1.0f;
        float m_PlayerCurrentHealth = 100.0f;
        float m_PlayerMaxHealth = 100.0f;
        float m_WaveCountdown = 0.0f;
        int m_EnemiesRemaining = 0;
        float m_DashCooldown = 0.0f;
        bool m_DashReady = true;
        
        // Notification timers
        float m_KillStreakTimer = 0.0f;
        char m_KillStreakText[64] = "";
        float m_LevelUpTimer = 0.0f;

        // Active buffs display
        std::vector<ActivePowerUpEffect> m_ActiveBuffs;

        // Damage numbers
        std::vector<DamageNumber> m_DamageNumbers;

        // Wave notification
        WaveNotification m_WaveNotification;

        // Combo display
        float m_ComboTimer = 0.0f;
        char m_ComboText[64] = "";
    };

} // namespace Game
