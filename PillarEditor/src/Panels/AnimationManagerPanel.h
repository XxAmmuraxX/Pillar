#pragma once

#include "EditorPanel.h"
#include "Pillar/ECS/Systems/AnimationSystem.h"
#include "Pillar/ECS/Components/Rendering/AnimationClip.h"
#include <memory>
#include <string>

namespace PillarEditor
{
    /**
     * AnimationManagerPanel - Manages animation clips for the scene
     * 
     * Features:
     * - Load animation clips from JSON files
     * - View all loaded clips
     * - Delete clips
     * - Preview clip information (frames, duration, events)
     * - Create new clips programmatically
     */
    class AnimationManagerPanel : public EditorPanel
    {
    public:
        AnimationManagerPanel();
        virtual ~AnimationManagerPanel() = default;

        void OnImGuiRender() override;

        // Set the animation system to manage
        void SetAnimationSystem(Pillar::AnimationSystem* system) { m_AnimationSystem = system; }

    private:
        void DrawToolbar();
        void DrawClipList();
        void DrawClipDetails(Pillar::AnimationClip* clip);
        void DrawLoadDialog();
        void DrawCreateClipDialog();

    private:
        Pillar::AnimationSystem* m_AnimationSystem = nullptr;
        
        // UI state
        int m_SelectedClipIndex = -1;
        char m_LoadPathBuffer[256] = "";
        bool m_ShowLoadDialog = false;
        bool m_ShowCreateDialog = false;
        
        // Create clip dialog state
        char m_NewClipNameBuffer[128] = "";
        bool m_NewClipLoop = true;
        float m_NewClipSpeed = 1.0f;
    };
}
