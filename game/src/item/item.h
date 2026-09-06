#pragma once

#include <cstdint>

namespace Gameplay
{
    struct sItemType
    {
        enum Enum : uint32_t
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
        enum Enum : uint32_t
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

    struct sItemId
    {
        enum Enum : uint32_t
        {
            HealthPotion,
            ManaPotion,

            ForestHelmet,
            ForestChest,
            ForestRing,
            ForestLegs,
            ForestBoots,

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
    };

    struct sItemStack
    {
        sItemId::Enum   item    = sItemId::Undefined;
        uint32_t        amount  = 0;

        bool IsEmpty() const
        {
            return item == sItemId::Undefined || amount == 0;
        }
    };
}