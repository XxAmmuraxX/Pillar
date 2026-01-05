#pragma once

#include <glm/glm.hpp>
#include <imgui.h>

namespace PillarEditor {
namespace Constants {

    // Inspector Panel
    namespace Inspector {
        constexpr float COLUMN_WIDTH_LABEL = 100.0f;
        constexpr float COLUMN_WIDTH_LABEL_WIDE = 120.0f;
        constexpr float COLUMN_WIDTH_LABEL_EXTRA_WIDE = 140.0f;
        
        constexpr float DRAG_SPEED_DEFAULT = 0.1f;
        constexpr float DRAG_SPEED_FAST = 1.0f;
        constexpr float DRAG_SPEED_SLOW = 0.5f;
        constexpr float DRAG_SPEED_PRECISE = 0.01f;
        constexpr float DRAG_SPEED_ROTATION = 0.05f;
        
        constexpr float RESET_VALUE_ZERO = 0.0f;
        constexpr float RESET_VALUE_ONE = 1.0f;
        
        constexpr float BUTTON_WIDTH = 30.0f;
        constexpr float SMALL_BUTTON_SIZE = 18.0f;
        constexpr float BUTTON_HEIGHT = 20.0f;
        
        // Component colors
        namespace Colors {
            constexpr ImVec4 BUTTON_X_NORMAL  = ImVec4(0.6f, 0.2f, 0.2f, 1.0f);  // Red
            constexpr ImVec4 BUTTON_X_HOVERED = ImVec4(0.7f, 0.3f, 0.3f, 1.0f);
            constexpr ImVec4 BUTTON_X_ACTIVE  = ImVec4(0.6f, 0.2f, 0.2f, 1.0f);
            
            constexpr ImVec4 BUTTON_Y_NORMAL  = ImVec4(0.2f, 0.6f, 0.2f, 1.0f);  // Green
            constexpr ImVec4 BUTTON_Y_HOVERED = ImVec4(0.3f, 0.7f, 0.3f, 1.0f);
            constexpr ImVec4 BUTTON_Y_ACTIVE  = ImVec4(0.2f, 0.6f, 0.2f, 1.0f);
            
            constexpr ImVec4 BUTTON_Z_NORMAL  = ImVec4(0.2f, 0.2f, 0.6f, 1.0f);  // Blue
            constexpr ImVec4 BUTTON_Z_HOVERED = ImVec4(0.3f, 0.3f, 0.7f, 1.0f);
            constexpr ImVec4 BUTTON_Z_ACTIVE  = ImVec4(0.2f, 0.2f, 0.6f, 1.0f);
        }
    }
    
    // Viewport Panel
    namespace Viewport {
        constexpr float GRID_LINE_THICKNESS = 1.0f;
        constexpr float GRID_MAJOR_SPACING = 1.0f;
        constexpr float GRID_MINOR_SPACING = 0.5f;
        
        constexpr ImVec4 GRID_COLOR = ImVec4(0.3f, 0.3f, 0.3f, 0.5f);
        constexpr ImVec4 GRID_AXIS_X_COLOR = ImVec4(0.8f, 0.2f, 0.2f, 0.8f);  // Red
        constexpr ImVec4 GRID_AXIS_Y_COLOR = ImVec4(0.2f, 0.8f, 0.2f, 0.8f);  // Green
        
        constexpr ImVec4 SELECTION_COLOR = ImVec4(1.0f, 0.7f, 0.0f, 1.0f);    // Orange
        constexpr float SELECTION_LINE_THICKNESS = 2.0f;
        constexpr float SELECTION_OUTLINE_OFFSET = 0.1f;
        
        constexpr float GIZMO_BUTTON_SIZE = 35.0f;
        constexpr float GIZMO_TOOLBAR_PADDING = 10.0f;
        
        constexpr ImVec4 BACKGROUND_COLOR = ImVec4(0.12f, 0.12f, 0.15f, 1.0f);
        
        // Entity visualization defaults
        constexpr float DEFAULT_ENTITY_SIZE = 0.5f;
        constexpr float PLAYER_ENTITY_SIZE = 0.5f;
        constexpr float ENEMY_ENTITY_SIZE = 0.4f;
        constexpr float BULLET_ENTITY_SIZE = 0.15f;
        constexpr float XPGEM_ENTITY_SIZE = 0.2f;
    }
    
    // Scene Hierarchy
    namespace Hierarchy {
        constexpr float TREE_NODE_INDENT = 20.0f;
        constexpr float ENTITY_BUTTON_SIZE = 20.0f;
        constexpr float ROW_HEIGHT = 22.0f;
    }
    
    // Content Browser
    namespace ContentBrowser {
        constexpr float THUMBNAIL_SIZE = 80.0f;
        constexpr float PADDING = 8.0f;
        constexpr float THUMBNAIL_PADDING = 16.0f;
    }
    
    // Console Panel
    namespace Console {
        constexpr int MAX_LOG_ENTRIES = 1000;
        constexpr float TIMESTAMP_WIDTH = 80.0f;
    }
    
    // Auto-save
    namespace AutoSave {
        constexpr float DEFAULT_INTERVAL = 300.0f;  // 5 minutes in seconds
        constexpr float MIN_INTERVAL = 60.0f;       // 1 minute
        constexpr float MAX_INTERVAL = 1800.0f;     // 30 minutes
    }
    
    // Editor Performance
    namespace Performance {
        constexpr size_t MAX_UNDO_HISTORY = 100;
        constexpr float FRAMEBUFFER_RESIZE_THRESHOLD = 1.0f;  // Minimum pixel difference
    }
    
    // ImGui Styling
    namespace UI {
        constexpr float PADDING = 4.0f;
        constexpr float INDENT = 16.0f;
        constexpr float SPACING = 8.0f;
        constexpr float SEPARATOR_THICKNESS = 1.0f;
        
        constexpr ImVec2 FRAME_PADDING = ImVec2(4.0f, 4.0f);
        constexpr ImVec2 ITEM_SPACING = ImVec2(8.0f, 4.0f);
        constexpr ImVec2 BUTTON_SIZE_SMALL = ImVec2(20.0f, 20.0f);
        constexpr ImVec2 BUTTON_SIZE_MEDIUM = ImVec2(30.0f, 30.0f);
        constexpr ImVec2 BUTTON_SIZE_LARGE = ImVec2(40.0f, 40.0f);
    }

}  // namespace Constants
}  // namespace PillarEditor
