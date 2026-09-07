#pragma once

#include "spellStats.h"

#include <cstdint>

namespace Gameplay
{
    struct sSpellAugment
    {
        enum Enum
        {
            Multishot,
            Pierce,
            DamageBonus,
            ExtraArea,
            ExtraDuration,
            LowerCooldown,

            NumberOfElements,
            Undefined = -1
        };
    };

    struct sSpellAugmentDefinition
    {
        sSpellAugment::Enum id  = sSpellAugment::Undefined;
        const char* pName       = "";
        uint8_t maxStacks       = 1;

        sSpellStats statModifier{};
    };
}
