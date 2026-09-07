#pragma once 

#include "spellAugment.h"
#include "spellDefinition.h"
#include "spellId.h"

#include <array>
#include <cstdint>

namespace Gameplay
{
    class cSpellInstance
    {
        public:

            void                Init(const sSpellDefinition& _rSpellDefinition); 
            sSpellId::Enum      GetSpellId()                                     const;
            const sSpellStats&  GetSpellStats()                                  const;
            uint8_t             GetAugmentCount(sSpellAugment::Enum _augment)    const;
            bool                ApplyAugment(sSpellAugment::Enum _augment);      
            void                Update(float _deltaTime);                        
            bool                IsOnCooldown()                                   const;
            float               GetCooldownRemaining()                           const;
            void                StartCooldown();

        private:

            void RebuildStats();

        private:

            sSpellId::Enum m_spellId = sSpellId::Undefined;

            sSpellStats m_baseStats{};
            sSpellStats m_stats{};

            std::array<uint8_t, sSpellAugment::NumberOfElements> m_augmentCounts{};

            float m_cooldownRemaining = 0.0f;
        };
}
