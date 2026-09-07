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
            static constexpr size_t c_numberOfAugmentChoices = 3;

        public:

            void Begin();
            void Update(float _deltaTime);
            bool GrantSpell(sSpellId::Enum _spellId);
            bool SetSpellSlot(size_t _slotIndex, sSpellId::Enum _spellId);
            bool StartSpellCooldown(size_t _slotIndex);
            uint32_t GrantExperience(uint32_t _experience);
            bool SelectAugment(sSpellAugment::Enum _augment);

            const cSpellInstance* TryGetSpell(sSpellId::Enum _spellId) const;
            const cSpellInstance* GetSpellInSlot(size_t _slotIndex) const;
            const std::array<sSpellId::Enum, c_numberOfSpellSlots>& GetSpellSlots() const;

            bool HasPendingAugmentSelection() const;
            bool CanSelectAugment(sSpellAugment::Enum _augment) const;
            uint8_t GetAugmentCount(sSpellAugment::Enum _augment) const;
            uint32_t GetPendingAugmentSelections() const;
            const std::array<sSpellAugment::Enum, c_numberOfAugmentChoices>& GetAugmentChoices() const;
            uint32_t GetExperience() const;
            uint32_t GetExperienceToNextLevel() const;
            uint32_t GetLevel() const;

        private:

            bool HasAvailableAugment() const;
            void RefreshAugmentChoices();

        private:

            std::array<cSpellInstance, sSpellId::NumberOfElements> m_spells{};
            std::array<sSpellId::Enum, c_numberOfSpellSlots> m_spellSlots{};
            std::array<uint8_t, sSpellAugment::NumberOfElements> m_augmentCounts{};
            std::array<sSpellAugment::Enum, c_numberOfAugmentChoices> m_augmentChoices{};

            uint32_t m_xp = 0;
            uint32_t m_level = 1;
            uint32_t m_pendingAugmentSelections = 0;
            size_t m_nextAugmentChoice = 0;
    };
}
