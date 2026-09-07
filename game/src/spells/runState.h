#pragma once

#include "spellInstance.h"

#include <array>
#include <cstdint>

namespace Gameplay
{
    class cRunState
    {
        public:

            static constexpr size_t c_numberOfSpellSlots = 6;

        public:

            void Begin();
            void Update(float _deltaTime);
            bool GrantSpell(sSpellId::Enum _spellId);
            bool SetSpellSlot(size_t _slotIndex, sSpellId::Enum _spellId);
            bool StartSpellCooldown(size_t _slotIndex);

            const cSpellInstance* TryGetSpell(sSpellId::Enum _spellId) const;
            const cSpellInstance* GetSpellInSlot(size_t _slotIndex) const;
            const std::array<sSpellId::Enum, c_numberOfSpellSlots>& GetSpellSlots() const;

        private:

            std::array<cSpellInstance, sSpellId::NumberOfElements> m_spells{};
            std::array<sSpellId::Enum, c_numberOfSpellSlots> m_spellSlots{};
            uint32_t m_xp = 0;
            uint32_t m_level = 1;
    };
}
