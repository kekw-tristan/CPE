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
            sSpellDefinition{ sSpellId::Fireball,       World::sBossId::ForestCrawler,   sItemId::Fireball,       sSpellCastType::Projectile,       "Fireball",        { 25.0f, 1.00f, 13.0f, 2.5f, 0.8f, 1 } },
            sSpellDefinition{ sSpellId::StoneShard,     World::sBossId::ForestBrute,     sItemId::StoneShard,     sSpellCastType::ConeProjectile,   "Stone Cone",      { 32.0f, 1.20f, 10.0f, 2.2f, 0.6f, 1 } },
            sSpellDefinition{ sSpellId::ThornBurst,     World::sBossId::ForestThornwolf, sItemId::ThornBurst,     sSpellCastType::Projectile,       "Thorn Burst",     { 18.0f, 0.85f, 15.0f, 1.8f, 0.6f, 3 } },
            sSpellDefinition{ sSpellId::SporeOrb,       World::sBossId::ForestSporecap,  sItemId::SporeOrb,       sSpellCastType::SporeProjectile,  "Spore Orb",       { 20.0f, 1.10f, 8.0f, 3.5f, 2.0f, 1 } },

            sSpellDefinition{ sSpellId::SandLance,      World::sBossId::DesertLancer,    sItemId::SandLance,      sSpellCastType::Projectile,       "Sand Lance",      { 35.0f, 1.10f, 18.0f, 1.8f, 0.5f, 1 } },
            sSpellDefinition{ sSpellId::SunDisk,        World::sBossId::DesertSentinel,  sItemId::SunDisk,        sSpellCastType::Projectile,       "Sun Disk",        { 28.0f, 1.25f, 12.0f, 2.8f, 1.0f, 1 } },
            sSpellDefinition{ sSpellId::MirageBolt,     World::sBossId::DesertStalker,   sItemId::MirageBolt,     sSpellCastType::Projectile,       "Mirage Bolt",     { 16.0f, 0.70f, 17.0f, 1.5f, 0.5f, 2 } },
            sSpellDefinition{ sSpellId::ScorpionVolley, World::sBossId::DesertScorpion,  sItemId::ScorpionVolley, sSpellCastType::Projectile,       "Scorpion Volley", { 14.0f, 1.00f, 14.0f, 2.0f, 0.5f, 5 } },

            sSpellDefinition{ sSpellId::FrostShard,     World::sBossId::IceWraith,       sItemId::FrostShard,     sSpellCastType::Projectile,       "Frost Shard",     { 24.0f, 0.90f, 14.0f, 2.5f, 0.7f, 1 } },
            sSpellDefinition{ sSpellId::GlacialOrb,     World::sBossId::IceGolem,        sItemId::GlacialOrb,     sSpellCastType::Projectile,       "Glacial Orb",     { 38.0f, 1.40f, 8.0f, 3.5f, 1.2f, 1 } },
            sSpellDefinition{ sSpellId::HailStorm,      World::sBossId::IceHarpy,        sItemId::HailStorm,      sSpellCastType::Projectile,       "Hail Storm",      { 12.0f, 0.80f, 16.0f, 1.6f, 0.4f, 6 } },
            sSpellDefinition{ sSpellId::CrystalWall,    World::sBossId::IceTitan,        sItemId::CrystalWall,    sSpellCastType::Projectile,       "Crystal Wall",    { 40.0f, 1.50f, 7.0f, 3.0f, 1.4f, 1 } },

            sSpellDefinition{ sSpellId::EmberBolt,      World::sBossId::LavaImp,         sItemId::EmberBolt,      sSpellCastType::Projectile,       "Ember Bolt",      { 22.0f, 0.75f, 16.0f, 2.0f, 0.6f, 1 } },
            sSpellDefinition{ sSpellId::MagmaBurst,     World::sBossId::LavaGolem,       sItemId::MagmaBurst,     sSpellCastType::Projectile,       "Magma Burst",     { 45.0f, 1.60f, 9.0f, 2.8f, 1.3f, 1 } },
            sSpellDefinition{ sSpellId::FlameWheel,     World::sBossId::LavaWyrm,        sItemId::FlameWheel,     sSpellCastType::Projectile,       "Flame Wheel",     { 17.0f, 0.95f, 15.0f, 2.4f, 0.7f, 4 } },
            sSpellDefinition{ sSpellId::Meteor,         World::sBossId::LavaTitan,       sItemId::Meteor,         sSpellCastType::Projectile,       "Meteor",          { 55.0f, 2.00f, 7.0f, 3.0f, 1.5f, 1 } }
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
            SpellManager::sBossDefinition{ World::sBossId::ForestCrawler,   World::sBiomeType::Forest,  World::sEnemyType::ForestCrawler,   sSpellId::Fireball },
            SpellManager::sBossDefinition{ World::sBossId::ForestBrute,     World::sBiomeType::Forest,  World::sEnemyType::ForestBrute,     sSpellId::StoneShard },
            SpellManager::sBossDefinition{ World::sBossId::ForestThornwolf, World::sBiomeType::Forest,  World::sEnemyType::ForestThornwolf, sSpellId::ThornBurst },
            SpellManager::sBossDefinition{ World::sBossId::ForestSporecap,  World::sBiomeType::Forest,  World::sEnemyType::ForestSporecap,  sSpellId::SporeOrb },

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
