#pragma once

#include <cstdint>

namespace Gameplay
{
    struct sItemType
    {
        enum Enum
        {
            Item,
            Armor,
            Usable,
            Spell,

            NumberOfElements,
            Undefined = -1
        };
    };

    struct sArmorSlot
    {
        enum Enum 
        {
            Head,
            Chest,
            Ring,
            Legs,
            Boots,

            NumberOfElements,
            Undefined = -1
        };
    };

    struct sItemRarity
    {
        enum Enum
        {
            Common,
            Rare,
            Legendary,

            Undefined = -1
        };
    };

    struct sItemId
    {
        enum Enum 
        {
            HealthPotion,
            ManaPotion,

            ForestHelmet,
            ForestChest,
            ForestRing,
            ForestLegs,
            ForestBoots,

            ArcaneOrb,
            Fireball,
            StoneShard,
            Dash,
            SporeOrb,

            SandLance,
            SunDisk,
            MirageBolt,
            ScorpionVolley,

            FrostShard,
            GlacialOrb,
            HailStorm,
            CrystalWall,

            EmberBolt,
            MagmaBurst,
            FlameWheel,
            Meteor,

            NumberOfElements,

            Undefined = -1
        };
    };

    struct sItemDefinition
    {
        uint32_t            id          = 0;
        const char*         pName       = "";
        sItemType::Enum     type        = sItemType::Undefined;
        sArmorSlot::Enum    armorSlot   = sArmorSlot::Undefined;
        uint32_t            maxStack    = 1;
        uint32_t            baseArmor   = 0;
    };

    struct sItemStack
    {
        sItemId::Enum       item    = sItemId::Undefined;
        uint32_t            amount  = 0;
        sItemRarity::Enum   rarity  = sItemRarity::Undefined;
        uint32_t            armor   = 0;

        bool IsEmpty() const
        {
            return item == sItemId::Undefined || amount == 0;
        }
    };
}
