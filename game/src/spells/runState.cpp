#include "runState.h"

#include "spellManager.h"

// -------------------------------------------------------------------------------------------------------------------------

namespace Gameplay
{

    // -------------------------------------------------------------------------------------------------------------------------

    void cRunState::Begin()
    {
        m_spells = {};
        m_spellSlots.fill(sSpellId::Undefined);
        m_xp = 0;
        m_level = 1;
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
}

// -------------------------------------------------------------------------------------------------------------------------
