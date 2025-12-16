#pragma once

#include <glm/glm.hpp>
#include <vector>

namespace BallGame {

    // ============================================
    // Golf Ball Component - Player controlled ball
    // ============================================
    struct GolfBallComponent
    {
        int ShotCount = 0;              // Strokes taken this level
        float MaxPower = 12.0f;         // Maximum launch velocity
        float MinPower = 2.0f;          // Minimum launch velocity
        bool IsMoving = false;          // Can only shoot when stopped
        bool InGoal = false;            // Level complete flag
        glm::vec2 LastShotPosition{0.0f}; // For display/tracking

        // Physics properties (applied once on creation)
        static constexpr float Radius = 0.3f;
        static constexpr float Restitution = 0.7f;   // Bounciness
        static constexpr float Friction = 0.1f;
        static constexpr float LinearDamping = 0.8f; // So ball stops eventually
    };

    // ============================================
    // Goal Component - Target hole
    // ============================================
    struct GoalComponent
    {
        float CaptureRadius = 0.6f;     // Ball must be within this distance
        float CaptureSpeed = 2.5f;      // Ball must be slower than this
        int ParScore = 3;               // Par for this level
        bool Captured = false;          // Has ball been captured?
        
        static constexpr float VisualRadius = 0.5f;
    };

    // ============================================
    // Wall Component - Bounce surfaces
    // ============================================
    struct WallComponent
    {
        bool IsVisible = true;
        glm::vec4 Color{0.3f, 0.3f, 0.35f, 1.0f};
    };

    // ============================================
    // Level State - Current game state
    // ============================================
    enum class GameState
    {
        MainMenu,       // Main menu screen
        Aiming,         // Player is aiming
        BallMoving,     // Ball is in motion
        LevelComplete,  // Ball reached goal
        Paused
    };

    // ============================================
    // Level Data - Definition for a level
    // ============================================
    struct LevelData
    {
        const char* Name = "Unnamed Level";
        int LevelNumber = 1;
        int Par = 3;
        glm::vec2 BallStart{0.0f, 0.0f};
        glm::vec2 GoalPosition{5.0f, 0.0f};
        
        // Walls are defined as {position, halfExtents, rotation}
        struct WallDef
        {
            glm::vec2 Position;
            glm::vec2 HalfExtents;
            float Rotation = 0.0f;
        };
        
        std::vector<WallDef> Walls;

        // Optional gravity wells that attract or repel the ball
        struct GravityWellDef
        {
            glm::vec2 Position{0.0f};
            float Radius = 4.0f;       // Influence radius
            float Strength = 22.0f;    // Force strength (positive = attract)
            bool IsRepulsor = false;   // When true, pushes instead of pulls
        };

        std::vector<GravityWellDef> GravityWells;

        // Optional boost pads that apply an instant shove along Direction
        struct BoostPadDef
        {
            glm::vec2 Position{0.0f};
            glm::vec2 Size{1.6f, 1.0f};   // Full size of the pad area
            glm::vec2 Direction{1.0f, 0.0f};
            float Strength = 11.0f;       // Impulse magnitude
        };

        std::vector<BoostPadDef> BoostPads;

        // Optional moving platforms (kinematic walls) that shuttle back and forth
        struct MovingPlatformDef
        {
            glm::vec2 Start{0.0f};
            glm::vec2 End{2.0f, 0.0f};
            glm::vec2 HalfExtents{0.7f, 0.35f};
            float Speed = 2.0f;           // Units per second
            float PauseTime = 0.4f;       // Pause at each end
        };

        std::vector<MovingPlatformDef> MovingPlatforms;
    };

    // ============================================
    // Gravity Well Component - runtime data
    // ============================================
    struct GravityWellComponent
    {
        float Radius = 4.0f;
        float Strength = 22.0f;
        bool IsRepulsor = false;
    };

    // ============================================
    // Boost Pad Component - runtime data
    // ============================================
    struct BoostPadComponent
    {
        glm::vec2 Size{1.6f, 1.0f};
        glm::vec2 Direction{1.0f, 0.0f};
        float Strength = 11.0f;
        float Cooldown = 0.0f; // Prevents re-trigger spam when inside pad
    };

    // ============================================
    // Moving Platform Component - runtime data
    // ============================================
    struct MovingPlatformComponent
    {
        glm::vec2 Start{0.0f};
        glm::vec2 End{2.0f, 0.0f};
        glm::vec2 HalfExtents{0.7f, 0.35f};
        float Speed = 2.0f;
        float PauseTime = 0.4f;
        float PauseTimer = 0.0f;
        bool Forward = true;
    };

} // namespace BallGame
