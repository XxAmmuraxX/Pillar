#pragma once

#include <string>
#include <vector>
#include <functional>
#include <fstream>
#include <algorithm>
#include <random>
#include <Pillar/Logger.h>

namespace Game {

    // Game states for state machine
    enum class GameStateType
    {
        MainMenu,
        Playing,
        Paused,
        PerkSelection,
        GameOver
    };

    // Weapon types available in the game
    enum class WeaponType
    {
        Pistol,         // Starting weapon
        Shotgun,        // Spread shot, unlocked at XP level
        SMG,            // Fast fire rate
        Rifle,          // High damage, slow fire
        Laser,          // Piercing beam
        Count
    };

    // Perk types that can be selected between waves
    enum class PerkType
    {
        // Offensive
        DamageUp,           // +25% damage
        FireRateUp,         // +20% fire rate
        BulletSpeedUp,      // +30% bullet speed
        PierceShot,         // Bullets pierce 1 extra enemy
        ExplosiveRounds,    // Bullets explode on impact
        
        // Defensive
        MaxHealthUp,        // +25 max HP
        Regeneration,       // Heal 1 HP per second
        DamageReduction,    // -15% damage taken
        DodgeChance,        // 10% chance to avoid damage
        
        // Utility
        MoveSpeedUp,        // +15% move speed
        XPMagnet,           // Larger pickup radius
        LuckyDrops,         // +25% power-up drop rate
        
        Count
    };

    struct PerkInfo
    {
        PerkType Type;
        std::string Name;
        std::string Description;
        int MaxLevel;
        int CurrentLevel = 0;
    };

    struct WeaponStats
    {
        WeaponType Type;
        std::string Name;
        float Damage;
        float FireRate;
        float BulletSpeed;
        float Spread;
        int BulletsPerShot;
        int XPCostToUnlock;
        bool Unlocked = false;
    };

    struct PlayerStats
    {
        // Base stats (modified by perks)
        float BaseDamageMultiplier = 1.0f;
        float BaseFireRateMultiplier = 1.0f;
        float BaseBulletSpeedMultiplier = 1.0f;
        float BaseMoveSpeedMultiplier = 1.0f;
        float DamageReduction = 0.0f;
        float DodgeChance = 0.0f;
        float RegenPerSecond = 0.0f;
        int ExtraPierce = 0;
        bool ExplosiveRounds = false;
        float XPPickupRadius = 1.5f;
        float DropRateBonus = 0.0f;
        int BonusMaxHealth = 0;

        void Reset()
        {
            BaseDamageMultiplier = 1.0f;
            BaseFireRateMultiplier = 1.0f;
            BaseBulletSpeedMultiplier = 1.0f;
            BaseMoveSpeedMultiplier = 1.0f;
            DamageReduction = 0.0f;
            DodgeChance = 0.0f;
            RegenPerSecond = 0.0f;
            ExtraPierce = 0;
            ExplosiveRounds = false;
            XPPickupRadius = 1.5f;
            DropRateBonus = 0.0f;
            BonusMaxHealth = 0;
        }
    };

    struct GameStats
    {
        int Score = 0;
        int TotalKills = 0;
        int CurrentXP = 0;
        int XPToNextLevel = 100;
        int PlayerLevel = 1;
        int WaveReached = 1;
        float PlayTime = 0.0f;
        int BossesKilled = 0;

        void Reset()
        {
            Score = 0;
            TotalKills = 0;
            CurrentXP = 0;
            XPToNextLevel = 100;
            PlayerLevel = 1;
            WaveReached = 1;
            PlayTime = 0.0f;
            BossesKilled = 0;
        }

        void AddXP(int amount)
        {
            CurrentXP += amount;
            while (CurrentXP >= XPToNextLevel)
            {
                CurrentXP -= XPToNextLevel;
                PlayerLevel++;
                XPToNextLevel = static_cast<int>(100 * std::pow(1.2f, PlayerLevel - 1));
                PIL_INFO("Level Up! Now level {}", PlayerLevel);
            }
        }
    };

    struct HighScoreEntry
    {
        std::string Name;
        int Score;
        int Wave;
        int Kills;
    };

    /**
     * GameState - Manages global game state, progression, and persistence
     */
    class GameState
    {
    public:
        static GameState& Instance()
        {
            static GameState instance;
            return instance;
        }

        void Init()
        {
            InitializeWeapons();
            InitializePerks();
            LoadHighScores();
            m_CurrentState = GameStateType::MainMenu;
        }

        void Reset()
        {
            m_Stats.Reset();
            m_PlayerStats.Reset();
            
            // Reset perks
            for (auto& perk : m_AvailablePerks)
                perk.CurrentLevel = 0;
            
            // Reset weapons (keep only pistol unlocked)
            for (auto& weapon : m_Weapons)
                weapon.Unlocked = (weapon.Type == WeaponType::Pistol);
            
            m_CurrentWeapon = WeaponType::Pistol;
            m_CurrentState = GameStateType::MainMenu;
        }

        // State Management
        GameStateType GetState() const { return m_CurrentState; }
        void SetState(GameStateType state) 
        { 
            m_PreviousState = m_CurrentState;
            m_CurrentState = state; 
        }
        GameStateType GetPreviousState() const { return m_PreviousState; }

        // Stats
        GameStats& GetStats() { return m_Stats; }
        const GameStats& GetStats() const { return m_Stats; }
        PlayerStats& GetPlayerStats() { return m_PlayerStats; }
        const PlayerStats& GetPlayerStats() const { return m_PlayerStats; }

        // Weapons
        const std::vector<WeaponStats>& GetWeapons() const { return m_Weapons; }
        WeaponStats& GetWeapon(WeaponType type) { return m_Weapons[static_cast<int>(type)]; }
        WeaponType GetCurrentWeapon() const { return m_CurrentWeapon; }
        void SetCurrentWeapon(WeaponType type) { m_CurrentWeapon = type; }
        
        bool CanUnlockWeapon(WeaponType type) const
        {
            const auto& weapon = m_Weapons[static_cast<int>(type)];
            return !weapon.Unlocked && m_Stats.PlayerLevel >= weapon.XPCostToUnlock;
        }

        void UnlockWeapon(WeaponType type)
        {
            m_Weapons[static_cast<int>(type)].Unlocked = true;
            PIL_INFO("Unlocked weapon: {}", m_Weapons[static_cast<int>(type)].Name);
        }

        std::vector<WeaponType> GetUnlockedWeapons() const
        {
            std::vector<WeaponType> unlocked;
            for (const auto& w : m_Weapons)
            {
                if (w.Unlocked)
                    unlocked.push_back(w.Type);
            }
            return unlocked;
        }

        // Perks
        std::vector<PerkInfo>& GetPerks() { return m_AvailablePerks; }
        const std::vector<PerkInfo>& GetPerks() const { return m_AvailablePerks; }
        
        std::vector<PerkInfo*> GetRandomPerks(int count)
        {
            std::vector<PerkInfo*> available;
            for (auto& perk : m_AvailablePerks)
            {
                if (perk.CurrentLevel < perk.MaxLevel)
                    available.push_back(&perk);
            }
            
            // Shuffle using modern C++ random
            static std::mt19937 rng{ std::random_device{}() };
            std::shuffle(available.begin(), available.end(), rng);
            if (available.size() > static_cast<size_t>(count))
                available.resize(count);
            
            return available;
        }

        void ApplyPerk(PerkType type)
        {
            auto& perk = m_AvailablePerks[static_cast<int>(type)];
            if (perk.CurrentLevel >= perk.MaxLevel) return;
            
            perk.CurrentLevel++;
            
            // Apply stat changes
            switch (type)
            {
                case PerkType::DamageUp:
                    m_PlayerStats.BaseDamageMultiplier += 0.25f;
                    break;
                case PerkType::FireRateUp:
                    m_PlayerStats.BaseFireRateMultiplier += 0.20f;
                    break;
                case PerkType::BulletSpeedUp:
                    m_PlayerStats.BaseBulletSpeedMultiplier += 0.30f;
                    break;
                case PerkType::PierceShot:
                    m_PlayerStats.ExtraPierce++;
                    break;
                case PerkType::ExplosiveRounds:
                    m_PlayerStats.ExplosiveRounds = true;
                    break;
                case PerkType::MaxHealthUp:
                    m_PlayerStats.BonusMaxHealth += 25;
                    break;
                case PerkType::Regeneration:
                    m_PlayerStats.RegenPerSecond += 1.0f;
                    break;
                case PerkType::DamageReduction:
                    m_PlayerStats.DamageReduction += 0.15f;
                    break;
                case PerkType::DodgeChance:
                    m_PlayerStats.DodgeChance += 0.10f;
                    break;
                case PerkType::MoveSpeedUp:
                    m_PlayerStats.BaseMoveSpeedMultiplier += 0.15f;
                    break;
                case PerkType::XPMagnet:
                    m_PlayerStats.XPPickupRadius += 2.0f;
                    break;
                case PerkType::LuckyDrops:
                    m_PlayerStats.DropRateBonus += 0.25f;
                    break;
                default:
                    break;
            }
            
            PIL_INFO("Applied perk: {} (Level {})", perk.Name, perk.CurrentLevel);
        }

        // High Scores
        const std::vector<HighScoreEntry>& GetHighScores() const { return m_HighScores; }
        
        bool IsHighScore(int score) const
        {
            return m_HighScores.size() < 10 || score > m_HighScores.back().Score;
        }

        void AddHighScore(const std::string& name, int score, int wave, int kills)
        {
            HighScoreEntry entry{ name, score, wave, kills };
            m_HighScores.push_back(entry);
            std::sort(m_HighScores.begin(), m_HighScores.end(),
                [](const auto& a, const auto& b) { return a.Score > b.Score; });
            if (m_HighScores.size() > 10)
                m_HighScores.resize(10);
            SaveHighScores();
        }

        void SaveHighScores()
        {
            std::ofstream file("swarm_slayer_scores.dat");
            if (file.is_open())
            {
                for (const auto& entry : m_HighScores)
                {
                    file << entry.Name << "," << entry.Score << "," 
                         << entry.Wave << "," << entry.Kills << "\n";
                }
            }
        }

        void LoadHighScores()
        {
            m_HighScores.clear();
            std::ifstream file("swarm_slayer_scores.dat");
            if (file.is_open())
            {
                std::string line;
                while (std::getline(file, line))
                {
                    HighScoreEntry entry;
                    size_t pos1 = line.find(',');
                    size_t pos2 = line.find(',', pos1 + 1);
                    size_t pos3 = line.find(',', pos2 + 1);
                    if (pos1 != std::string::npos && pos2 != std::string::npos)
                    {
                        entry.Name = line.substr(0, pos1);
                        entry.Score = std::stoi(line.substr(pos1 + 1, pos2 - pos1 - 1));
                        entry.Wave = std::stoi(line.substr(pos2 + 1, pos3 - pos2 - 1));
                        entry.Kills = std::stoi(line.substr(pos3 + 1));
                        m_HighScores.push_back(entry);
                    }
                }
            }
        }

        // Callbacks for perk selection
        void SetOnPerkSelected(std::function<void()> callback) { m_OnPerkSelected = callback; }
        void TriggerPerkSelected() { if (m_OnPerkSelected) m_OnPerkSelected(); }

    private:
        GameState() = default;

        void InitializeWeapons()
        {
            m_Weapons.clear();
            m_Weapons.push_back({ WeaponType::Pistol, "Pistol", 10.0f, 5.0f, 20.0f, 2.0f, 1, 0, true });
            m_Weapons.push_back({ WeaponType::Shotgun, "Shotgun", 8.0f, 2.0f, 15.0f, 25.0f, 5, 3, false });
            m_Weapons.push_back({ WeaponType::SMG, "SMG", 6.0f, 12.0f, 18.0f, 8.0f, 1, 5, false });
            m_Weapons.push_back({ WeaponType::Rifle, "Rifle", 35.0f, 1.5f, 30.0f, 0.5f, 1, 8, false });
            m_Weapons.push_back({ WeaponType::Laser, "Laser", 15.0f, 8.0f, 40.0f, 0.0f, 1, 12, false });
        }

        void InitializePerks()
        {
            m_AvailablePerks.clear();
            m_AvailablePerks.push_back({ PerkType::DamageUp, "Damage Up", "+25% bullet damage", 5, 0 });
            m_AvailablePerks.push_back({ PerkType::FireRateUp, "Fire Rate Up", "+20% fire rate", 5, 0 });
            m_AvailablePerks.push_back({ PerkType::BulletSpeedUp, "Bullet Speed", "+30% bullet speed", 3, 0 });
            m_AvailablePerks.push_back({ PerkType::PierceShot, "Pierce Shot", "Bullets pierce +1 enemy", 3, 0 });
            m_AvailablePerks.push_back({ PerkType::ExplosiveRounds, "Explosive Rounds", "Bullets explode on impact", 1, 0 });
            m_AvailablePerks.push_back({ PerkType::MaxHealthUp, "Max Health Up", "+25 max health", 5, 0 });
            m_AvailablePerks.push_back({ PerkType::Regeneration, "Regeneration", "Heal 1 HP per second", 3, 0 });
            m_AvailablePerks.push_back({ PerkType::DamageReduction, "Armor", "-15% damage taken", 4, 0 });
            m_AvailablePerks.push_back({ PerkType::DodgeChance, "Dodge", "10% chance to avoid damage", 3, 0 });
            m_AvailablePerks.push_back({ PerkType::MoveSpeedUp, "Swift", "+15% move speed", 4, 0 });
            m_AvailablePerks.push_back({ PerkType::XPMagnet, "Magnet", "Larger pickup radius", 3, 0 });
            m_AvailablePerks.push_back({ PerkType::LuckyDrops, "Lucky", "+25% drop chance", 3, 0 });
        }

    private:
        GameStateType m_CurrentState = GameStateType::MainMenu;
        GameStateType m_PreviousState = GameStateType::MainMenu;
        
        GameStats m_Stats;
        PlayerStats m_PlayerStats;
        
        std::vector<WeaponStats> m_Weapons;
        WeaponType m_CurrentWeapon = WeaponType::Pistol;
        
        std::vector<PerkInfo> m_AvailablePerks;
        std::vector<HighScoreEntry> m_HighScores;
        
        std::function<void()> m_OnPerkSelected;
    };

} // namespace Game
