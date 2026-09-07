#pragma once

#include "spellDefinition.h"
#include "spellId.h"
#include "spellAugment.h"

#include "../world/biome.h"

namespace Gameplay
{
    namespace SpellManager
    {
        const sSpellDefinition& GetSpell(sSpellId::Enum _spellId);
        const sSpellAugmentDefinition& GetAugment(sSpellAugment::Enum _augment);
        sSpellId::Enum GetSpellId(sItemId::Enum _item);

        struct sBossDefinition
        {
            World::sBossId::Enum    id          = World::sBossId::Undefined;
            World::sBiomeType::Enum biome       = World::sBiomeType::Forest;
            World::sEnemyType::Enum enemyType   = World::sEnemyType::Undefined;

            sSpellId::Enum spellReward = sSpellId::Undefined;
        };

        const sBossDefinition& GetBoss(World::sBossId::Enum _bossId);
    }
}
