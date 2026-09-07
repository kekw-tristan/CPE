#include "runState.h"

#include "spellManager.h"

#include <algorithm>

// -------------------------------------------------------------------------------------------------------------------------

namespace Gameplay
{

    constexpr uint32_t c_baseExperienceToNextLevel = 100;
    constexpr uint32_t c_experiencePerLevel         = 50;

    // -------------------------------------------------------------------------------------------------------------------------

    void cRunState::Begin()
    {
        m_spells = {};
        m_spellSlots.fill(sSpellId::Undefined);
        m_augmentCounts.fill(0);
        m_augmentChoices.fill(sSpellAugment::Undefined);
        m_xp = 0;
        m_level = 1;
        m_pendingAugmentSelections = 0;
        m_nextAugmentChoice = 0;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cRunState::Update(float _deltaTime)
    {
        for (cSpellInstance& spell : m_spells)
        {
            if (spell.GetSpellId() != sSpellId::Undefined)
                spell.Update(_deltaTime);
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

    bool cRunState::GrantSpell(sSpellId::Enum _spellId)
    {
        if (_spellId < 0 || _spellId >= sSpellId::NumberOfElements)
            return false;

        cSpellInstance& spell = m_spells[static_cast<size_t>(_spellId)];

        if (spell.GetSpellId() != sSpellId::Undefined)
            return false;

        spell.Init(SpellManager::GetSpell(_spellId));

        for (size_t augmentIndex = 0; augmentIndex < m_augmentCounts.size(); ++augmentIndex)
        {
            for (uint8_t stack = 0; stack < m_augmentCounts[augmentIndex]; ++stack)
                spell.ApplyAugment(static_cast<sSpellAugment::Enum>(augmentIndex));
        }

        return true;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    bool cRunState::SetSpellSlot(size_t _slotIndex, sSpellId::Enum _spellId)
    {
        if (_slotIndex >= m_spellSlots.size())
            return false;

        if (_spellId != sSpellId::Undefined && TryGetSpell(_spellId) == nullptr)
            return false;

        m_spellSlots[_slotIndex] = _spellId;

        return true;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    uint32_t cRunState::GrantExperience(uint32_t _experience)
    {
        uint32_t levelUps = 0;

        while (_experience > 0)
        {
            const uint32_t experienceToNextLevel = GetExperienceToNextLevel();
            const uint32_t experienceRemaining = experienceToNextLevel - m_xp;

            if (_experience < experienceRemaining)
            {
                m_xp += _experience;
                break;
            }

            _experience -= experienceRemaining;
            m_xp = 0;
            ++m_level;
            ++levelUps;

            if (HasAvailableAugment())
                ++m_pendingAugmentSelections;
        }

        if (m_pendingAugmentSelections > 0)
            RefreshAugmentChoices();

        return levelUps;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    bool cRunState::SelectAugment(sSpellAugment::Enum _augment)
    {
        if (!CanSelectAugment(_augment))
            return false;

        const size_t augmentIndex = static_cast<size_t>(_augment);
        ++m_augmentCounts[augmentIndex];

        for (cSpellInstance& spell : m_spells)
        {
            if (spell.GetSpellId() != sSpellId::Undefined)
                spell.ApplyAugment(_augment);
        }

        --m_pendingAugmentSelections;

        if (m_pendingAugmentSelections > 0)
            RefreshAugmentChoices();
        else
            m_augmentChoices.fill(sSpellAugment::Undefined);

        return true;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    bool cRunState::StartSpellCooldown(size_t _slotIndex)
    {
        if (_slotIndex >= m_spellSlots.size())
            return false;

        const sSpellId::Enum spellId = m_spellSlots[_slotIndex];
        if (spellId < 0 || spellId >= sSpellId::NumberOfElements)
            return false;

        cSpellInstance& spell = m_spells[static_cast<size_t>(spellId)];
        if (spell.GetSpellId() != spellId)
            return false;

        spell.StartCooldown();

        return true;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    const cSpellInstance* cRunState::TryGetSpell(sSpellId::Enum _spellId) const
    {
        if (_spellId < 0 || _spellId >= sSpellId::NumberOfElements)
            return nullptr;

        const cSpellInstance& spell = m_spells[static_cast<size_t>(_spellId)];

        return spell.GetSpellId() == _spellId ? &spell : nullptr;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    const cSpellInstance* cRunState::GetSpellInSlot(size_t _slotIndex) const
    {
        if (_slotIndex >= m_spellSlots.size())
            return nullptr;

        return TryGetSpell(m_spellSlots[_slotIndex]);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    const std::array<sSpellId::Enum, cRunState::c_numberOfSpellSlots>& cRunState::GetSpellSlots() const
    {
        return m_spellSlots;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    bool cRunState::HasPendingAugmentSelection() const
    {
        return m_pendingAugmentSelections > 0;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    bool cRunState::CanSelectAugment(sSpellAugment::Enum _augment) const
    {
        if (!HasPendingAugmentSelection() || _augment < 0 || _augment >= sSpellAugment::NumberOfElements)
            return false;

        const bool isOffered = std::find(m_augmentChoices.begin(), m_augmentChoices.end(), _augment) != m_augmentChoices.end();
        if (!isOffered)
            return false;

        const size_t augmentIndex = static_cast<size_t>(_augment);
        return m_augmentCounts[augmentIndex] < SpellManager::GetAugment(_augment).maxStacks;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    uint8_t cRunState::GetAugmentCount(sSpellAugment::Enum _augment) const
    {
        if (_augment < 0 || _augment >= sSpellAugment::NumberOfElements)
            return 0;

        return m_augmentCounts[static_cast<size_t>(_augment)];
    }

    // -------------------------------------------------------------------------------------------------------------------------

    uint32_t cRunState::GetPendingAugmentSelections() const
    {
        return m_pendingAugmentSelections;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    const std::array<sSpellAugment::Enum, cRunState::c_numberOfAugmentChoices>& cRunState::GetAugmentChoices() const
    {
        return m_augmentChoices;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    uint32_t cRunState::GetExperience() const
    {
        return m_xp;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    uint32_t cRunState::GetExperienceToNextLevel() const
    {
        return c_baseExperienceToNextLevel + (m_level - 1) * c_experiencePerLevel;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    uint32_t cRunState::GetLevel() const
    {
        return m_level;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    bool cRunState::HasAvailableAugment() const
    {
        for (size_t augmentIndex = 0; augmentIndex < m_augmentCounts.size(); ++augmentIndex)
        {
            const sSpellAugment::Enum augment = static_cast<sSpellAugment::Enum>(augmentIndex);
            if (m_augmentCounts[augmentIndex] < SpellManager::GetAugment(augment).maxStacks)
                return true;
        }

        return false;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cRunState::RefreshAugmentChoices()
    {
        m_augmentChoices.fill(sSpellAugment::Undefined);

        size_t choiceIndex = 0;
        for (size_t offset = 0; offset < m_augmentCounts.size() && choiceIndex < m_augmentChoices.size(); ++offset)
        {
            const size_t augmentIndex = (m_nextAugmentChoice + offset) % m_augmentCounts.size();
            const sSpellAugment::Enum augment = static_cast<sSpellAugment::Enum>(augmentIndex);

            if (m_augmentCounts[augmentIndex] >= SpellManager::GetAugment(augment).maxStacks)
                continue;

            m_augmentChoices[choiceIndex++] = augment;
        }

        m_nextAugmentChoice = (m_nextAugmentChoice + 1) % m_augmentCounts.size();
    }

    // -------------------------------------------------------------------------------------------------------------------------
}

// -------------------------------------------------------------------------------------------------------------------------
