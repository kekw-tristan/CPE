#include "spellManager.h"

#include <array>
#include <cassert>

// -------------------------------------------------------------------------------------------------------------------------

namespace Gameplay
{

    // -------------------------------------------------------------------------------------------------------------------------

    namespace
    {

        // -------------------------------------------------------------------------------------------------------------------------

        const std::array<sSpellDefinition, sSpellId::NumberOfElements> c_spells =
        {
            sSpellDefinition{ sSpellId::ArcaneOrb,      World::sBossId::Undefined,       sItemId::ArcaneOrb,      sSpellCastType::Projectile,       0.0f, "Arcane Orb",      { 12.0f, 0.45f, 13.0f, 2.5f, 0.8f, 1 } },
            
            sSpellDefinition{ sSpellId::Fireball,       World::sBossId::ForestCrawler,   sItemId::Fireball,       sSpellCastType::Projectile,       0.0f, "Fireball",        { 25.0f, 1.00f, 13.0f, 2.5f, 0.8f, 1 } },
            sSpellDefinition{ sSpellId::StoneShard,     World::sBossId::ForestBrute,     sItemId::StoneShard,     sSpellCastType::ConeProjectile,   15.0f, "Stone Cone",      { 32.0f, 1.20f, 10.0f, 2.2f, 0.6f, 1 } },
            sSpellDefinition{ sSpellId::ThornBurst,     World::sBossId::ForestThornwolf, sItemId::ThornBurst,     sSpellCastType::Projectile,       14.0f, "Thorn Burst",     { 18.0f, 0.85f, 15.0f, 1.8f, 0.6f, 3 } },
            sSpellDefinition{ sSpellId::SporeOrb,       World::sBossId::ForestSporecap,  sItemId::SporeOrb,       sSpellCastType::SporeProjectile,  18.0f, "Poison Mushroom", { 36.0f, 1.50f, 12.0f, 4.5f, 4.0f, 1 } },

            sSpellDefinition{ sSpellId::SandLance,      World::sBossId::DesertLancer,    sItemId::SandLance,      sSpellCastType::Projectile,       14.0f, "Sand Lance",      { 35.0f, 1.10f, 18.0f, 1.8f, 0.5f, 1 } },
            sSpellDefinition{ sSpellId::SunDisk,        World::sBossId::DesertSentinel,  sItemId::SunDisk,        sSpellCastType::Projectile,       18.0f, "Sun Disk",        { 28.0f, 1.25f, 12.0f, 2.8f, 1.0f, 1 } },
            sSpellDefinition{ sSpellId::MirageBolt,     World::sBossId::DesertStalker,   sItemId::MirageBolt,     sSpellCastType::Projectile,       15.0f, "Mirage Bolt",     { 16.0f, 0.70f, 17.0f, 1.5f, 0.5f, 2 } },
            sSpellDefinition{ sSpellId::ScorpionVolley, World::sBossId::DesertScorpion,  sItemId::ScorpionVolley, sSpellCastType::Projectile,       20.0f, "Scorpion Volley", { 14.0f, 1.00f, 14.0f, 2.0f, 0.5f, 5 } },

            sSpellDefinition{ sSpellId::FrostShard,     World::sBossId::IceWraith,       sItemId::FrostShard,     sSpellCastType::Projectile,       16.0f, "Frost Shard",     { 24.0f, 0.90f, 14.0f, 2.5f, 0.7f, 1 } },
            sSpellDefinition{ sSpellId::GlacialOrb,     World::sBossId::IceGolem,        sItemId::GlacialOrb,     sSpellCastType::Projectile,       22.0f, "Glacial Orb",     { 38.0f, 1.40f, 8.0f, 3.5f, 1.2f, 1 } },
            sSpellDefinition{ sSpellId::HailStorm,      World::sBossId::IceHarpy,        sItemId::HailStorm,      sSpellCastType::Projectile,       20.0f, "Hail Storm",      { 12.0f, 0.80f, 16.0f, 1.6f, 0.4f, 6 } },
            sSpellDefinition{ sSpellId::CrystalWall,    World::sBossId::IceTitan,        sItemId::CrystalWall,    sSpellCastType::Projectile,       24.0f, "Crystal Wall",    { 40.0f, 1.50f, 7.0f, 3.0f, 1.4f, 1 } },

            sSpellDefinition{ sSpellId::EmberBolt,      World::sBossId::LavaImp,         sItemId::EmberBolt,      sSpellCastType::Projectile,       17.0f, "Ember Bolt",      { 22.0f, 0.75f, 16.0f, 2.0f, 0.6f, 1 } },
            sSpellDefinition{ sSpellId::MagmaBurst,     World::sBossId::LavaGolem,       sItemId::MagmaBurst,     sSpellCastType::Projectile,       25.0f, "Magma Burst",     { 45.0f, 1.60f, 9.0f, 2.8f, 1.3f, 1 } },
            sSpellDefinition{ sSpellId::FlameWheel,     World::sBossId::LavaWyrm,        sItemId::FlameWheel,     sSpellCastType::Projectile,       22.0f, "Flame Wheel",     { 17.0f, 0.95f, 15.0f, 2.4f, 0.7f, 4 } },
            sSpellDefinition{ sSpellId::Meteor,         World::sBossId::LavaTitan,       sItemId::Meteor,         sSpellCastType::Projectile,       30.0f, "Meteor",          { 55.0f, 2.00f, 7.0f, 3.0f, 1.5f, 1 } }
        };

        // -------------------------------------------------------------------------------------------------------------------------

        constexpr std::array<sSpellAugmentDefinition, sSpellAugment::NumberOfElements> c_augments =
        {
            sSpellAugmentDefinition{ sSpellAugment::Multishot,          "Multishot",        4, { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1, 0 } },
            sSpellAugmentDefinition{ sSpellAugment::Pierce,             "Pierce",           4, { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0, 1 } },
            sSpellAugmentDefinition{ sSpellAugment::DamageBonus,        "Damage Bonus",     5, { 8.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0, 0 } },
            sSpellAugmentDefinition{ sSpellAugment::ExtraArea,          "Extra Area",       4, { 0.0f, 0.0f, 0.0f, 0.0f, 0.2f, 0 } },
            sSpellAugmentDefinition{ sSpellAugment::ExtraDuration,      "Extra Duration",   4, { 0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0 } },
            sSpellAugmentDefinition{ sSpellAugment::LowerCooldown,      "Lower Cooldown",   4, { 0.0f, -0.12f, 0.0f, 0.0f, 0.0f, 0 } }
        };

        // -------------------------------------------------------------------------------------------------------------------------

        constexpr std::array<SpellManager::sBossDefinition, World::sBossId::NumberOfElements> c_bosses =
        {
            SpellManager::sBossDefinition{ World::sBossId::ForestCrawler,   World::sBiomeType::Forest,  World::sEnemyType::ForestThornshooter, sSpellId::Fireball },
            SpellManager::sBossDefinition{ World::sBossId::ForestBrute,     World::sBiomeType::Forest,  World::sEnemyType::ForestBarkguard,    sSpellId::StoneShard },
            SpellManager::sBossDefinition{ World::sBossId::ForestThornwolf, World::sBiomeType::Forest,  World::sEnemyType::ForestRootcharger,  sSpellId::ThornBurst },
            SpellManager::sBossDefinition{ World::sBossId::ForestSporecap,  World::sBiomeType::Forest,  World::sEnemyType::ForestSporecap,     sSpellId::SporeOrb },

            SpellManager::sBossDefinition{ World::sBossId::DesertLancer,    World::sBiomeType::Desert,  World::sEnemyType::Undefined, sSpellId::SandLance },
            SpellManager::sBossDefinition{ World::sBossId::DesertSentinel,  World::sBiomeType::Desert,  World::sEnemyType::Undefined, sSpellId::SunDisk },
            SpellManager::sBossDefinition{ World::sBossId::DesertStalker,   World::sBiomeType::Desert,  World::sEnemyType::Undefined, sSpellId::MirageBolt },
            SpellManager::sBossDefinition{ World::sBossId::DesertScorpion,  World::sBiomeType::Desert,  World::sEnemyType::Undefined, sSpellId::ScorpionVolley },

            SpellManager::sBossDefinition{ World::sBossId::IceWraith,       World::sBiomeType::Ice,     World::sEnemyType::Undefined, sSpellId::FrostShard },
            SpellManager::sBossDefinition{ World::sBossId::IceGolem,        World::sBiomeType::Ice,     World::sEnemyType::Undefined, sSpellId::GlacialOrb },
            SpellManager::sBossDefinition{ World::sBossId::IceHarpy,        World::sBiomeType::Ice,     World::sEnemyType::Undefined, sSpellId::HailStorm },
            SpellManager::sBossDefinition{ World::sBossId::IceTitan,        World::sBiomeType::Ice,     World::sEnemyType::Undefined, sSpellId::CrystalWall },

            SpellManager::sBossDefinition{ World::sBossId::LavaImp,         World::sBiomeType::Lava,    World::sEnemyType::Undefined, sSpellId::EmberBolt },
            SpellManager::sBossDefinition{ World::sBossId::LavaGolem,       World::sBiomeType::Lava,    World::sEnemyType::Undefined, sSpellId::MagmaBurst },
            SpellManager::sBossDefinition{ World::sBossId::LavaWyrm,        World::sBiomeType::Lava,    World::sEnemyType::Undefined, sSpellId::FlameWheel },
            SpellManager::sBossDefinition{ World::sBossId::LavaTitan,       World::sBiomeType::Lava,    World::sEnemyType::Undefined, sSpellId::Meteor }
        };

        // -------------------------------------------------------------------------------------------------------------------------

    }

    // -------------------------------------------------------------------------------------------------------------------------

    namespace SpellManager
    {

        // -------------------------------------------------------------------------------------------------------------------------

        const sSpellDefinition& GetSpell(sSpellId::Enum _spellId)
        {
            assert(_spellId >= 0 && _spellId < sSpellId::NumberOfElements);
            return c_spells[static_cast<size_t>(_spellId)];
        }

        // -------------------------------------------------------------------------------------------------------------------------

        const sSpellAugmentDefinition& GetAugment(sSpellAugment::Enum _augment)
        {
            assert(_augment >= 0 && _augment < sSpellAugment::NumberOfElements);
            return c_augments[static_cast<size_t>(_augment)];
        }

        // -------------------------------------------------------------------------------------------------------------------------

        sSpellId::Enum GetSpellId(sItemId::Enum _item)
        {
            for (const sSpellDefinition& spell : c_spells)
            {
                if (spell.inventoryItem == _item)
                    return spell.id;
            }

            return sSpellId::Undefined;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        const sBossDefinition& GetBoss(World::sBossId::Enum _bossId)
        {
            assert(_bossId >= 0 && _bossId < World::sBossId::NumberOfElements);
            return c_bosses[static_cast<size_t>(_bossId)];
        }

        // -------------------------------------------------------------------------------------------------------------------------

    }

    // -------------------------------------------------------------------------------------------------------------------------
}

// -------------------------------------------------------------------------------------------------------------------------
