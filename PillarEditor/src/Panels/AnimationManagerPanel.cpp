#include "AnimationManagerPanel.h"
#include "Pillar/Utils/AssetManager.h"
#include "ConsolePanel.h"
#include <imgui.h>
#include <algorithm>

namespace PillarEditor
{
    AnimationManagerPanel::AnimationManagerPanel()
        : EditorPanel("Animation Manager")
    {
    }

    void AnimationManagerPanel::OnImGuiRender()
    {
        ImGui::Begin("Animation Manager", &m_Visible);

        if (!m_AnimationSystem)
        {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "No AnimationSystem set!");
            ImGui::Text("The scene needs an AnimationSystem to manage clips.");
            ImGui::End();
            return;
        }

        DrawToolbar();
        ImGui::Separator();
        
        ImGui::TextWrapped("Animation clips are registered with the AnimationSystem. "
                          "Clips contain frames with texture paths and UV coordinates.");
        ImGui::Spacing();
        ImGui::TextDisabled("Workflow:");
        ImGui::BulletText("Create clips programmatically (see AnimationDemoLayer)");
        ImGui::BulletText("Or load from .anim.json files");
        ImGui::BulletText("Entities reference clips by name in AnimationComponent");
        ImGui::Separator();
        
        // Two-column layout: Clip list | Details
        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, 250.0f);
        
        DrawClipList();
        
        ImGui::NextColumn();
        
        // Draw details for selected clip
        if (m_SelectedClipIndex >= 0 && m_SelectedClipIndex < m_AnimationSystem->GetClipCount())
        {
            // Get clip by index (we'll need to iterate to find it)
            int idx = 0;
            Pillar::AnimationClip* selectedClip = nullptr;
            // Note: We don't have direct index access, so we'll store clip name instead
            // For now, just show a placeholder
            ImGui::Text("Clip details will be shown here");
        }
        
        ImGui::Columns(1);

        ImGui::End();
        
        // Dialogs (modals)
        DrawLoadDialog();
        DrawCreateClipDialog();
    }

    void AnimationManagerPanel::DrawToolbar()
    {
        if (ImGui::Button("Load Clip"))
        {
            m_ShowLoadDialog = true;
        }
        
        ImGui::SameLine();
        
        if (ImGui::Button("Create Clip"))
        {
            m_ShowCreateDialog = true;
        }
        
        ImGui::SameLine();
        
        if (ImGui::Button("Clear All"))
        {
            m_AnimationSystem->ClearLibrary();
            m_SelectedClipIndex = -1;
            ConsolePanel::Log("Cleared all animation clips", LogLevel::Info);
        }
        
        ImGui::SameLine();
        ImGui::Text("| %zu clips loaded", m_AnimationSystem->GetClipCount());
    }

    void AnimationManagerPanel::DrawClipList()
    {
        ImGui::BeginChild("ClipList", ImVec2(0, 0), true);
        
        ImGui::TextDisabled("Loaded Clips:");
        ImGui::Separator();
        
        if (m_AnimationSystem->GetClipCount() == 0)
        {
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No clips loaded");
            ImGui::Text("Use 'Load Clip' or 'Create Clip'");
        }
        else
        {
            // We need to expose a way to iterate clips
            // For now, show count
            ImGui::Text("Total Clips: %zu", m_AnimationSystem->GetClipCount());
            ImGui::Separator();
            ImGui::TextDisabled("Note: Individual clip listing");
            ImGui::TextDisabled("requires iterator support");
        }
        
        ImGui::EndChild();
    }

    void AnimationManagerPanel::DrawClipDetails(Pillar::AnimationClip* clip)
    {
        if (!clip)
            return;
            
        ImGui::BeginChild("ClipDetails");
        
        ImGui::Text("Clip: %s", clip->Name.c_str());
        ImGui::Separator();
        
        ImGui::Text("Frames: %zu", clip->Frames.size());
        ImGui::Text("Duration: %.2f seconds", clip->GetDuration());
        ImGui::Text("Loop: %s", clip->Loop ? "Yes" : "No");
        ImGui::Text("Speed: %.2fx", clip->PlaybackSpeed);
        ImGui::Text("Events: %zu", clip->Events.size());
        
        ImGui::Spacing();
        ImGui::TextDisabled("Frames:");
        ImGui::Separator();
        
        for (size_t i = 0; i < clip->Frames.size(); ++i)
        {
            auto& frame = clip->Frames[i];
            if (ImGui::TreeNode((void*)(intptr_t)i, "Frame %zu", i))
            {
                ImGui::Text("Texture: %s", frame.TexturePath.c_str());
                ImGui::Text("Duration: %.3f sec", frame.Duration);
                ImGui::Text("UV Min: (%.2f, %.2f)", frame.UVMin.x, frame.UVMin.y);
                ImGui::Text("UV Max: (%.2f, %.2f)", frame.UVMax.x, frame.UVMax.y);
                ImGui::TreePop();
            }
        }
        
        if (!clip->Events.empty())
        {
            ImGui::Spacing();
            ImGui::TextDisabled("Events:");
            ImGui::Separator();
            
            for (size_t i = 0; i < clip->Events.size(); ++i)
            {
                auto& evt = clip->Events[i];
                ImGui::Text("Frame %d: '%s'", evt.FrameIndex, evt.EventName.c_str());
            }
        }
        
        ImGui::EndChild();
    }

    void AnimationManagerPanel::DrawLoadDialog()
    {
        if (m_ShowLoadDialog)
        {
            ImGui::OpenPopup("Load Animation Clip");
            m_ShowLoadDialog = false;
        }
        
        if (ImGui::BeginPopupModal("Load Animation Clip", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Enter animation file path (relative to assets/)");
            ImGui::Separator();
            
            ImGui::InputTextWithHint("##LoadPath", "animations/player_walk.anim.json", 
                                    m_LoadPathBuffer, sizeof(m_LoadPathBuffer));
            
            ImGui::Spacing();
            ImGui::TextDisabled("Animation files should be in JSON format (.anim.json)");
            ImGui::TextDisabled("Place them in assets/animations/");
            
            ImGui::Separator();
            
            if (ImGui::Button("Load", ImVec2(120, 0)))
            {
                if (strlen(m_LoadPathBuffer) > 0)
                {
                    std::string fullPath = Pillar::AssetManager::GetAssetPath(m_LoadPathBuffer);
                    
                    if (m_AnimationSystem->LoadAnimationClip(fullPath))
                    {
                        ConsolePanel::Log("Loaded animation clip: " + std::string(m_LoadPathBuffer), LogLevel::Info);
                        m_LoadPathBuffer[0] = '\0';
                        ImGui::CloseCurrentPopup();
                    }
                    else
                    {
                        ConsolePanel::Log("Failed to load animation clip: " + std::string(m_LoadPathBuffer), LogLevel::Error);
                    }
                }
            }
            
            ImGui::SameLine();
            
            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                m_LoadPathBuffer[0] = '\0';
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::EndPopup();
        }
    }

    void AnimationManagerPanel::DrawCreateClipDialog()
    {
        if (m_ShowCreateDialog)
        {
            ImGui::OpenPopup("Create Animation Clip");
            m_ShowCreateDialog = false;
        }
        
        if (ImGui::BeginPopupModal("Create Animation Clip", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Create a new animation clip programmatically");
            ImGui::Separator();
            
            ImGui::InputText("Clip Name", m_NewClipNameBuffer, sizeof(m_NewClipNameBuffer));
            ImGui::Checkbox("Loop", &m_NewClipLoop);
            ImGui::DragFloat("Playback Speed", &m_NewClipSpeed, 0.1f, 0.1f, 5.0f);
            
            ImGui::Spacing();
            ImGui::TextDisabled("Note: You'll need to add frames manually in code");
            ImGui::TextDisabled("or create a JSON file instead.");
            
            ImGui::Separator();
            
            if (ImGui::Button("Create", ImVec2(120, 0)))
            {
                if (strlen(m_NewClipNameBuffer) > 0)
                {
                    Pillar::AnimationClip newClip(m_NewClipNameBuffer);
                    newClip.Loop = m_NewClipLoop;
                    newClip.PlaybackSpeed = m_NewClipSpeed;
                    
                    m_AnimationSystem->RegisterClip(newClip);
                    ConsolePanel::Log("Created animation clip: " + std::string(m_NewClipNameBuffer), LogLevel::Info);
                    
                    m_NewClipNameBuffer[0] = '\0';
                    m_NewClipLoop = true;
                    m_NewClipSpeed = 1.0f;
                    ImGui::CloseCurrentPopup();
                }
            }
            
            ImGui::SameLine();
            
            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                m_NewClipNameBuffer[0] = '\0';
                m_NewClipLoop = true;
                m_NewClipSpeed = 1.0f;
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::EndPopup();
        }
    }
}
