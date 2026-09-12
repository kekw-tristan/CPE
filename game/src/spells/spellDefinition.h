#pragma once 

#include "spellStats.h"
#include "spellId.h"

#include "../item/item.h"
#include "../world/enemy/enemySpawn.h"

#include <string>

namespace Gameplay
{
    struct sSpellCastType
    {
        enum Enum
        {
            Projectile,
            ConeProjectile,
            SporeProjectile,
            Dash,

            NumberOfElements,
            Undefined = -1
        };
    };

    struct sSpellDefinition 
    { 
        sSpellId::Enum       id             = sSpellId::Undefined;
        World::sBossId::Enum sourceBoss     = World::sBossId::Undefined;
        sItemId::Enum        inventoryItem  = sItemId::Undefined;
        sSpellCastType::Enum castType       = sSpellCastType::Undefined;
        float                manaCost       = 0.0f;

        std::string          name;
        sSpellStats          baseStats; 
    };
}
