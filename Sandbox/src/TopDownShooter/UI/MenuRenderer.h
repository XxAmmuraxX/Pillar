#pragma once

#include <imgui.h>
#include <Pillar/Logger.h>
#include <Pillar/Application.h>
#include "../Core/GameState.h"

namespace Game {

    /**
     * MenuRenderer - Renders all menu UIs (Main Menu, Pause, Game Over, Perk Selection)
     * Uses ImGui for dark/gritty themed UI
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
            
            // ---- TOP-LEFT: Health, XP, Level ----
            {
                float panelX = 15.0f;
                float panelY = 15.0f;
                float barWidth = 180.0f;
                float barHeight = 18.0f;
                
                // Health bar background
                drawList->AddRectFilled(
                    ImVec2(panelX, panelY),
                    ImVec2(panelX + barWidth, panelY + barHeight),
                    IM_COL32(30, 30, 30, 200), 4.0f);
                
                // Health bar fill
                float healthFill = barWidth * m_PlayerHealthPercent;
                if (healthFill > 0)
                {
                    ImU32 healthColor = m_PlayerHealthPercent > 0.3f ? 
                        IM_COL32(200, 50, 50, 255) : IM_COL32(255, 80, 80, 255);
                    drawList->AddRectFilled(
                        ImVec2(panelX, panelY),
                        ImVec2(panelX + healthFill, panelY + barHeight),
                        healthColor, 4.0f);
                }
                
                // Health text
                char healthText[32];
                snprintf(healthText, sizeof(healthText), "HP: %.0f/%.0f", m_PlayerCurrentHealth, m_PlayerMaxHealth);
                drawList->AddText(ImVec2(panelX + 5, panelY + 2), IM_COL32(255, 255, 255, 255), healthText);
                
                // XP bar (smaller, below health)
                float xpY = panelY + barHeight + 5;
                float xpBarHeight = 12.0f;
                float xpPercent = static_cast<float>(stats.CurrentXP) / stats.XPToNextLevel;
                
                drawList->AddRectFilled(
                    ImVec2(panelX, xpY),
                    ImVec2(panelX + barWidth, xpY + xpBarHeight),
                    IM_COL32(20, 20, 40, 200), 3.0f);
                    
                if (xpPercent > 0)
                {
                    drawList->AddRectFilled(
                        ImVec2(panelX, xpY),
                        ImVec2(panelX + barWidth * xpPercent, xpY + xpBarHeight),
                        IM_COL32(80, 180, 255, 255), 3.0f);
                }
                
                char xpText[32];
                snprintf(xpText, sizeof(xpText), "LV %d - %d/%d XP", stats.PlayerLevel, stats.CurrentXP, stats.XPToNextLevel);
                drawList->AddText(ImVec2(panelX + 5, xpY), IM_COL32(200, 220, 255, 255), xpText);
            }
            
            // ---- TOP-CENTER: Wave Indicator ----
            {
                char waveText[32];
                snprintf(waveText, sizeof(waveText), "WAVE %d", stats.WaveReached);
                ImVec2 textSize = ImGui::CalcTextSize(waveText);
                float waveX = (displaySize.x - textSize.x * 1.5f) * 0.5f;
                
                // Background pill
                drawList->AddRectFilled(
                    ImVec2(waveX - 15, 10),
                    ImVec2(waveX + textSize.x * 1.5f + 15, 40),
                    IM_COL32(40, 30, 10, 200), 15.0f);
                    
                drawList->AddText(ImGui::GetFont(), ImGui::GetFontSize() * 1.5f,
                    ImVec2(waveX, 12), IM_COL32(255, 200, 50, 255), waveText);
            }
            
            // ---- TOP-RIGHT: Score & Kills ----
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
                
                // Background
                drawList->AddRectFilled(
                    ImVec2(panelX - 10, panelY - 5),
                    ImVec2(displaySize.x - 10, panelY + 40),
                    IM_COL32(30, 30, 30, 180), 5.0f);
                
                drawList->AddText(ImVec2(panelX, panelY), IM_COL32(255, 255, 80, 255), scoreText);
                drawList->AddText(ImVec2(panelX, panelY + 20), IM_COL32(200, 80, 80, 255), killsText);
            }
            
            // ---- BOTTOM-LEFT: Dash Indicator ----
            {
                float panelX = 15;
                float panelY = displaySize.y - 45;
                
                const char* dashText = m_DashReady ? "[DASH READY]" : "";
                char dashCooldownText[32] = "";
                if (!m_DashReady)
                    snprintf(dashCooldownText, sizeof(dashCooldownText), "[DASH: %.1fs]", m_DashCooldown);
                
                const char* displayText = m_DashReady ? dashText : dashCooldownText;
                ImU32 dashColor = m_DashReady ? IM_COL32(100, 200, 255, 255) : IM_COL32(128, 128, 128, 200);
                
                drawList->AddText(ImVec2(panelX, panelY), dashColor, displayText);
            }
            
            // ---- BOTTOM-CENTER: Weapon & Controls ----
            {
                auto currentWeapon = GameState::Instance().GetCurrentWeapon();
                auto& weapon = GameState::Instance().GetWeapon(currentWeapon);
                
                char weaponText[128];
                snprintf(weaponText, sizeof(weaponText), "[%s]  WASD:Move  LMB:Shoot  Space:Dash  1-5:Weapons", 
                    weapon.Name.c_str());
                
                ImVec2 textSize = ImGui::CalcTextSize(weaponText);
                float textX = (displaySize.x - textSize.x) * 0.5f;
                float textY = displaySize.y - 30;
                
                // Background
                drawList->AddRectFilled(
                    ImVec2(textX - 10, textY - 5),
                    ImVec2(textX + textSize.x + 10, textY + textSize.y + 5),
                    IM_COL32(20, 20, 20, 180), 5.0f);
                
                drawList->AddText(ImVec2(textX, textY), IM_COL32(150, 230, 150, 255), weaponText);
            }
            
            // ---- Kill Streak Notification (if active) ----
            if (m_KillStreakTimer > 0.0f)
            {
                float alpha = std::min(1.0f, m_KillStreakTimer * 2.0f);
                float scale = 1.5f + (1.0f - alpha) * 0.5f;
                
                ImVec2 textSize = ImGui::CalcTextSize(m_KillStreakText);
                float textX = (displaySize.x - textSize.x * scale) * 0.5f;
                float textY = displaySize.y * 0.3f;
                
                ImU32 streakColor = IM_COL32(255, 200, 50, static_cast<int>(alpha * 255));
                drawList->AddText(ImGui::GetFont(), ImGui::GetFontSize() * scale,
                    ImVec2(textX, textY), streakColor, m_KillStreakText);
            }
            
            // ---- Level Up Notification (if active) ----
            if (m_LevelUpTimer > 0.0f)
            {
                float alpha = std::min(1.0f, m_LevelUpTimer);
                float bounce = std::sin(m_LevelUpTimer * 10.0f) * 5.0f;
                
                const char* levelUpText = "LEVEL UP!";
                ImVec2 textSize = ImGui::CalcTextSize(levelUpText);
                float textX = (displaySize.x - textSize.x * 2.0f) * 0.5f;
                float textY = displaySize.y * 0.4f + bounce;
                
                ImU32 levelColor = IM_COL32(100, 200, 255, static_cast<int>(alpha * 255));
                drawList->AddText(ImGui::GetFont(), ImGui::GetFontSize() * 2.0f,
                    ImVec2(textX, textY), levelColor, levelUpText);
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

    private:
        void ApplyDarkTheme()
        {
            ImGuiStyle& style = ImGui::GetStyle();
            style.WindowRounding = 4.0f;
            style.FrameRounding = 2.0f;
            style.Colors[ImGuiCol_WindowBg] = ImVec4(0.05f, 0.05f, 0.08f, 0.95f);
            style.Colors[ImGuiCol_Button] = ImVec4(0.3f, 0.1f, 0.1f, 1.0f);
            style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.5f, 0.15f, 0.15f, 1.0f);
            style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.7f, 0.2f, 0.2f, 1.0f);
            style.Colors[ImGuiCol_TitleBg] = ImVec4(0.1f, 0.05f, 0.05f, 1.0f);
            style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.2f, 0.1f, 0.1f, 1.0f);
            style.Colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.1f, 0.1f, 1.0f);
            style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.25f, 0.15f, 0.15f, 1.0f);
            style.Colors[ImGuiCol_Header] = ImVec4(0.3f, 0.15f, 0.15f, 1.0f);
            style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.4f, 0.2f, 0.2f, 1.0f);
        }

        void RenderMainMenu()
        {
            ImVec2 displaySize = ImGui::GetIO().DisplaySize;
            
            // Full screen dark overlay
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(displaySize);
            ImGui::Begin("##MainMenuBG", nullptr,
                ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoInputs |
                ImGuiWindowFlags_NoDocking);
            ImGui::End();

            // Center window
            ImGui::SetNextWindowPos(ImVec2(displaySize.x * 0.5f, displaySize.y * 0.5f), 
                ImGuiCond_Always, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(400, 500));
            
            ImGui::Begin("##MainMenu", nullptr,
                ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoDocking);

            // Title
            ImGui::SetCursorPosX((400 - ImGui::CalcTextSize("SWARM SLAYER").x * 2.5f) * 0.5f);
            ImGui::SetWindowFontScale(2.5f);
            ImGui::TextColored(ImVec4(0.8f, 0.2f, 0.2f, 1.0f), "SWARM SLAYER");
            ImGui::SetWindowFontScale(1.0f);

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // Subtitle
            ImGui::SetCursorPosX((400 - ImGui::CalcTextSize("Survive the endless horde").x) * 0.5f);
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Survive the endless horde");

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

            // Full screen dark overlay
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(displaySize);
            ImGui::SetNextWindowBgAlpha(0.8f);
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

            // Title
            ImGui::SetCursorPosX((450 - ImGui::CalcTextSize("GAME OVER").x * 2.5f) * 0.5f);
            ImGui::SetWindowFontScale(2.5f);
            ImGui::TextColored(ImVec4(0.8f, 0.2f, 0.2f, 1.0f), "GAME OVER");
            ImGui::SetWindowFontScale(1.0f);

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // Stats
            ImGui::SetWindowFontScale(1.3f);
            ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.3f, 1.0f), "FINAL SCORE: %d", stats.Score);
            ImGui::SetWindowFontScale(1.0f);

            ImGui::Spacing();
            ImGui::Text("Wave Reached: %d", stats.WaveReached);
            ImGui::Text("Total Kills: %d", stats.TotalKills);
            ImGui::Text("Bosses Killed: %d", stats.BossesKilled);
            ImGui::Text("Player Level: %d", stats.PlayerLevel);
            
            int minutes = static_cast<int>(stats.PlayTime) / 60;
            int seconds = static_cast<int>(stats.PlayTime) % 60;
            ImGui::Text("Survival Time: %02d:%02d", minutes, seconds);

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // High score entry
            if (GameState::Instance().IsHighScore(stats.Score))
            {
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "NEW HIGH SCORE!");
                
                static char nameBuffer[32] = "Player";
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
    };

} // namespace Game
