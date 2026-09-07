#include "itemDatabase.h"

#include <array>
#include <cassert>
#include <cstddef>

// ---------------------------------------------------------------------------------------------------------------------

namespace Gameplay
{

    // ---------------------------------------------------------------------------------------------------------------------

    namespace
    {

        // ---------------------------------------------------------------------------------------------------------------------

        constexpr std::array<sItemDefinition, sItemId::NumberOfElements> c_itemDefinitions =
        {

            // ---------------------------------------------------------------------------------------------------------------------

            sItemDefinition
            {
                sItemId::HealthPotion,
                "Health Potion",
                sItemType::Usable,
                sArmorSlot::Undefined,
                10
            },

            // ---------------------------------------------------------------------------------------------------------------------

            sItemDefinition
            {
                sItemId::ManaPotion,
                "Mana Potion",
                sItemType::Usable,
                sArmorSlot::Undefined,
                10
            },

            // ---------------------------------------------------------------------------------------------------------------------

            sItemDefinition
            {
                sItemId::ForestHelmet,
                "Forest Helmet",
                sItemType::Armor,
                sArmorSlot::Head,
                1
            },

            // ---------------------------------------------------------------------------------------------------------------------

            sItemDefinition
            {
                sItemId::ForestChest,
                "Forest Chest",
                sItemType::Armor,
                sArmorSlot::Chest,
                1
            },

            // ---------------------------------------------------------------------------------------------------------------------

            sItemDefinition
            {
                sItemId::ForestRing,
                "Forest Ring",
                sItemType::Armor,
                sArmorSlot::Ring,
                1
            },

            // ---------------------------------------------------------------------------------------------------------------------

            sItemDefinition
            {
                sItemId::ForestLegs,
                "Forest Legs",
                sItemType::Armor,
                sArmorSlot::Legs,
                1
            },

            // ---------------------------------------------------------------------------------------------------------------------

            sItemDefinition
            {
                sItemId::ForestBoots,
                "Forest Boots",
                sItemType::Armor,
                sArmorSlot::Boots,
                1
            },

            // ---------------------------------------------------------------------------------------------------------------------

            sItemDefinition{ sItemId::Fireball,       "Fireball",        sItemType::Spell, sArmorSlot::Undefined, 1 },
            sItemDefinition{ sItemId::StoneShard,     "Stone Cone",      sItemType::Spell, sArmorSlot::Undefined, 1 },
            sItemDefinition{ sItemId::ThornBurst,     "Thorn Burst",     sItemType::Spell, sArmorSlot::Undefined, 1 },
            sItemDefinition{ sItemId::SporeOrb,       "Spore Orb",       sItemType::Spell, sArmorSlot::Undefined, 1 },
                                                                         
            sItemDefinition{ sItemId::SandLance,      "Sand Lance",      sItemType::Spell, sArmorSlot::Undefined, 1 },
            sItemDefinition{ sItemId::SunDisk,        "Sun Disk",        sItemType::Spell, sArmorSlot::Undefined, 1 },
            sItemDefinition{ sItemId::MirageBolt,     "Mirage Bolt",     sItemType::Spell, sArmorSlot::Undefined, 1 },
            sItemDefinition{ sItemId::ScorpionVolley, "Scorpion Volley", sItemType::Spell, sArmorSlot::Undefined, 1 },
                                                                         
            sItemDefinition{ sItemId::FrostShard,     "Frost Shard",     sItemType::Spell, sArmorSlot::Undefined, 1 },
            sItemDefinition{ sItemId::GlacialOrb,     "Glacial Orb",     sItemType::Spell, sArmorSlot::Undefined, 1 },
            sItemDefinition{ sItemId::HailStorm,      "Hail Storm",      sItemType::Spell, sArmorSlot::Undefined, 1 },
            sItemDefinition{ sItemId::CrystalWall,    "Crystal Wall",    sItemType::Spell, sArmorSlot::Undefined, 1 },
                                                                         
            sItemDefinition{ sItemId::EmberBolt,      "Ember Bolt",      sItemType::Spell, sArmorSlot::Undefined, 1 },
            sItemDefinition{ sItemId::MagmaBurst,     "Magma Burst",     sItemType::Spell, sArmorSlot::Undefined, 1 },
            sItemDefinition{ sItemId::FlameWheel,     "Flame Wheel",     sItemType::Spell, sArmorSlot::Undefined, 1 },
            sItemDefinition{ sItemId::Meteor,         "Meteor",          sItemType::Spell, sArmorSlot::Undefined, 1 }

            // ---------------------------------------------------------------------------------------------------------------------
        };

        // ---------------------------------------------------------------------------------------------------------------------
    }

    // ---------------------------------------------------------------------------------------------------------------------

    const sItemDefinition& GetItemDefinition(sItemId::Enum _item)
    {
        assert(_item >= 0);
        assert(_item < sItemId::NumberOfElements);

        return c_itemDefinitions[static_cast<size_t>(_item)];
    }

    // ---------------------------------------------------------------------------------------------------------------------
}

// ---------------------------------------------------------------------------------------------------------------------
