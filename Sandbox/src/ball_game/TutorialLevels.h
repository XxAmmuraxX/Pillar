#pragma once

#include "GameComponents.h"
#include <vector>

namespace BallGame {

    // ============================================
    // Tutorial Level Definitions
    // ============================================
    
    class TutorialLevels
    {
    public:
        static std::vector<LevelData> GetAllLevels()
        {
            std::vector<LevelData> levels;
            levels.push_back(CreateLevel1());
            levels.push_back(CreateLevel2());
            levels.push_back(CreateLevel3());
            levels.push_back(CreateLevel4());
            levels.push_back(CreateLevel5());
            levels.push_back(CreateLevel6());
            //levels.push_back(CreateLevel7());
            return levels;
        }

        static int GetLevelCount() { return 7; }

    private:
        // ----------------------------------------
        // Level 1: "First Shot"
        // Straight line from ball to goal
        // Teaches: aiming, power control
        // ----------------------------------------
        static LevelData CreateLevel1()
        {
            LevelData level;
            level.Name = "First Shot";
            level.LevelNumber = 1;
            level.Par = 1;
            level.BallStart = {-4.0f, 0.0f};
            level.GoalPosition = {4.0f, 0.0f};

            // Simple corridor walls
            // Top wall
            level.Walls.push_back({{0.0f, 2.0f}, {6.0f, 0.3f}, 0.0f});
            // Bottom wall
            level.Walls.push_back({{0.0f, -2.0f}, {6.0f, 0.3f}, 0.0f});
            // Left wall (behind start)
            level.Walls.push_back({{-5.5f, 0.0f}, {0.3f, 2.0f}, 0.0f});
            // Right wall (behind goal)
            level.Walls.push_back({{5.5f, 0.0f}, {0.3f, 2.0f}, 0.0f});

            return level;
        }

        // ----------------------------------------
        // Level 2: "Corner Pocket"
        // One wall bounce required
        // Teaches: wall physics, planning trajectory
        // ----------------------------------------
        static LevelData CreateLevel2()
        {
            LevelData level;
            level.Name = "Corner Pocket";
            level.LevelNumber = 2;
            level.Par = 2;
            level.BallStart = {-4.0f, -2.0f};
            level.GoalPosition = {4.0f, 2.0f};

            // Outer boundary
            level.Walls.push_back({{0.0f, 4.0f}, {6.0f, 0.3f}, 0.0f});   // Top
            level.Walls.push_back({{0.0f, -4.0f}, {6.0f, 0.3f}, 0.0f});  // Bottom
            level.Walls.push_back({{-5.5f, 0.0f}, {0.3f, 4.0f}, 0.0f});  // Left
            level.Walls.push_back({{5.5f, 0.0f}, {0.3f, 4.0f}, 0.0f});   // Right

            // Central blocker - forces bank shot
            level.Walls.push_back({{0.0f, 0.0f}, {2.5f, 0.3f}, 0.0f});

            return level;
        }

        // ----------------------------------------
        // Level 3: "The Maze"
        // Navigate through obstacles
        // Teaches: precision aiming, multiple shots
        // ----------------------------------------
        static LevelData CreateLevel3()
        {
            LevelData level;
            level.Name = "The Maze";
            level.LevelNumber = 3;
            level.Par = 3;
            level.BallStart = {-5.0f, 0.0f};
            level.GoalPosition = {5.0f, 0.0f};

            // Outer boundary
            level.Walls.push_back({{0.0f, 3.5f}, {7.0f, 0.3f}, 0.0f});   // Top
            level.Walls.push_back({{0.0f, -3.5f}, {7.0f, 0.3f}, 0.0f});  // Bottom
            level.Walls.push_back({{-6.5f, 0.0f}, {0.3f, 3.5f}, 0.0f});  // Left
            level.Walls.push_back({{6.5f, 0.0f}, {0.3f, 3.5f}, 0.0f});   // Right

            // Maze walls - create a winding path
            // First barrier from top
            level.Walls.push_back({{-3.0f, 1.5f}, {0.3f, 2.0f}, 0.0f});
            // Second barrier from bottom
            level.Walls.push_back({{0.0f, -1.5f}, {0.3f, 2.0f}, 0.0f});
            // Third barrier from top
            level.Walls.push_back({{3.0f, 1.5f}, {0.3f, 2.0f}, 0.0f});

            return level;
        }

        // ----------------------------------------
        // Level 4: "Gravity Garden"
        // Introduces gravity wells (attractors/repulsors)
        // Teaches: curved shots, timing, field control
        // ----------------------------------------
        static LevelData CreateLevel4()
        {
            LevelData level;
            level.Name = "Gravity Garden";
            level.LevelNumber = 4;
            level.Par = 4;
            level.BallStart = {-7.0f, -1.0f};
            level.GoalPosition = {7.0f, 1.0f};

            // Outer boundary
            level.Walls.push_back({{0.0f, 4.5f}, {8.5f, 0.3f}, 0.0f});   // Top
            level.Walls.push_back({{0.0f, -4.5f}, {8.5f, 0.3f}, 0.0f});  // Bottom
            level.Walls.push_back({{-8.5f, 0.0f}, {0.3f, 4.5f}, 0.0f});  // Left
            level.Walls.push_back({{8.5f, 0.0f}, {0.3f, 4.5f}, 0.0f});   // Right

            // Midline obstacles to encourage using wells
            level.Walls.push_back({{0.0f, 0.0f}, {0.25f, 1.4f}, 0.0f});
            level.Walls.push_back({{-2.5f, 2.0f}, {1.6f, 0.25f}, 0.0f});
            level.Walls.push_back({{2.5f, -2.0f}, {1.6f, 0.25f}, 0.0f});

            // Gravity wells: two attractors guiding path, one repulsor to dodge
            level.GravityWells.push_back({{-2.0f, 0.5f}, 3.8f, 32.0f, false});   // Pulls toward center
            level.GravityWells.push_back({{2.5f, 1.5f}, 3.4f, 28.0f, false});    // Pull near goal side
            level.GravityWells.push_back({{0.5f, -1.8f}, 3.0f, 26.0f, true});    // Repulsor to bend path

            return level;
        }

        // ----------------------------------------
        // Level 5: "Boost Boulevard"
        // Introduces directional boost pads to shape shots
        // Teaches: lining up boosts, chaining velocity control
        // ----------------------------------------
        static LevelData CreateLevel5()
        {
            LevelData level;
            level.Name = "Boost Boulevard";
            level.LevelNumber = 5;
            level.Par = 3;
            level.BallStart = {-7.5f, -0.5f};
            level.GoalPosition = {7.5f, 0.8f};

            // Outer boundary
            level.Walls.push_back({{0.0f, 4.0f}, {8.5f, 0.3f}, 0.0f});   // Top
            level.Walls.push_back({{0.0f, -4.0f}, {8.5f, 0.3f}, 0.0f});  // Bottom
            level.Walls.push_back({{-9.0f, 0.0f}, {0.3f, 4.0f}, 0.0f});  // Left
            level.Walls.push_back({{9.0f, 0.0f}, {0.3f, 4.0f}, 0.0f});   // Right

            // Mid-lane blockers to force boost usage
            level.Walls.push_back({{-2.0f, 1.6f}, {1.5f, 0.25f}, 0.0f});
            level.Walls.push_back({{2.0f, -1.6f}, {1.5f, 0.25f}, 0.0f});

            // Boost pads chain the path around the blockers
            level.BoostPads.push_back({{-5.5f, -0.2f}, {1.8f, 1.0f}, {1.0f, 0.15f}, 12.5f});  // Gentle right push
            level.BoostPads.push_back({{-1.0f, -0.6f}, {1.6f, 1.0f}, {0.65f, 0.4f}, 13.0f});  // Angle upward-right
            level.BoostPads.push_back({{3.5f, 0.9f}, {1.8f, 1.0f}, {0.85f, 0.2f}, 12.0f});   // Final redirect toward goal

            // Light attractor near goal to keep shots on line
            level.GravityWells.push_back({{6.5f, 1.0f}, 3.2f, 18.0f, false});

            return level;
        }

        // ----------------------------------------
        // Level 6: "Conveyor Crossing"
        // Introduces moving platforms plus boosts + wells
        // Teaches: timing shots with motion and force fields
        // ----------------------------------------
        static LevelData CreateLevel6()
        {
            LevelData level;
            level.Name = "Conveyor Crossing";
            level.LevelNumber = 6;
            level.Par = 4;
            level.BallStart = {-8.5f, -2.2f};
            level.GoalPosition = {8.0f, 2.4f};

            // Outer boundary
            level.Walls.push_back({{0.0f, 4.8f}, {9.5f, 0.3f}, 0.0f});   // Top
            level.Walls.push_back({{0.0f, -4.8f}, {9.5f, 0.3f}, 0.0f});  // Bottom
            level.Walls.push_back({{-10.0f, 0.0f}, {0.3f, 4.8f}, 0.0f}); // Left
            level.Walls.push_back({{10.0f, 0.0f}, {0.3f, 4.8f}, 0.0f});  // Right

            // Static pillars to break sight lines
            level.Walls.push_back({{-2.5f, 0.0f}, {0.35f, 1.3f}, 0.0f});
            level.Walls.push_back({{2.5f, 0.0f}, {0.35f, 1.3f}, 0.0f});

            // Moving platforms: two ferries that shuttle shots across the center channel
            level.MovingPlatforms.push_back({{-4.5f, -0.5f}, {-0.5f, -0.5f}, {1.2f, 0.35f}, 2.2f, 0.5f});
            level.MovingPlatforms.push_back({{0.5f, 0.9f}, {4.5f, 0.9f}, {1.2f, 0.35f}, 2.4f, 0.5f});

            // Boost pads to hop onto each ferry
            level.BoostPads.push_back({{-8.0f, -1.2f}, {1.8f, 1.1f}, {0.95f, 0.15f}, 13.5f});
            level.BoostPads.push_back({{-3.0f, 0.2f}, {1.6f, 1.0f}, {0.75f, 0.35f}, 12.5f});
            level.BoostPads.push_back({{2.5f, 1.8f}, {1.6f, 1.0f}, {0.85f, 0.15f}, 12.0f});

            // Wells: repulsor to keep players off the deep channel and attractor that hugs the goal
            level.GravityWells.push_back({{0.0f, -2.4f}, 3.4f, 26.0f, true});
            level.GravityWells.push_back({{6.5f, 2.3f}, 3.0f, 20.0f, false});

            return level;
        }

        // ----------------------------------------
        // Level 7: "Twilight Circuit"
        // Long, winding course using all mechanics
        // Prefers loading from JSON if present
        // ----------------------------------------
        static LevelData CreateLevel7()
        {
            LevelData level;
            level.Name = "Twilight Circuit";
            level.LevelNumber = 7;
            level.Par = 5;
            level.BallStart = {-11.5f, -1.8f};
            level.GoalPosition = {13.5f, 2.8f};

            // Keep definitions minimal; the runtime will deserialize from JSON if present.
            // Provide a light fallback so regen works if JSON is missing.
            level.Walls.push_back({{0.0f, 0.0f}, {6.0f, 0.3f}, 0.0f});
            level.GravityWells.push_back({{-7.5f, 1.5f}, 3.3f, 28.0f, false});
            level.GravityWells.push_back({{0.0f, -3.2f}, 2.9f, 24.0f, true});
            level.BoostPads.push_back({{-9.5f, -1.8f}, {1.8f, 1.0f}, {0.95f, 0.10f}, 12.5f});
            level.MovingPlatforms.push_back({{-6.5f, -0.4f}, {-2.0f, 0.2f}, {1.2f, 0.35f}, 2.4f, 0.5f});

            return level;
        }
    };

} // namespace BallGame
