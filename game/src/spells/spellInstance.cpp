#include "spellInstance.h"

#include "spellManager.h"

#include <algorithm>

// -------------------------------------------------------------------------------------------------------------------------

namespace Gameplay
{

    // -------------------------------------------------------------------------------------------------------------------------

    void cSpellInstance::Init(const sSpellDefinition& _rSpellDefinition)
    {
        m_spellId   = _rSpellDefinition.id;
        m_baseStats = _rSpellDefinition.baseStats;
        m_augmentCounts.fill(0);
        m_cooldownRemaining = 0.0f;

        RebuildStats();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    sSpellId::Enum cSpellInstance::GetSpellId() const
    {
        return m_spellId;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    const sSpellStats& cSpellInstance::GetSpellStats() const
    {
        return m_stats;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    uint8_t cSpellInstance::GetAugmentCount(sSpellAugment::Enum _augment) const
    {
        if (_augment < 0 || _augment >= sSpellAugment::NumberOfElements)
            return 0;

        return m_augmentCounts[static_cast<size_t>(_augment)];
    }

    // -------------------------------------------------------------------------------------------------------------------------

    bool cSpellInstance::ApplyAugment(sSpellAugment::Enum _augment)
    {
        if (m_spellId == sSpellId::Undefined || _augment < 0 || _augment >= sSpellAugment::NumberOfElements)
            return false;

        const sSpellAugmentDefinition& definition = SpellManager::GetAugment(_augment);
        uint8_t& count = m_augmentCounts[static_cast<size_t>(_augment)];

        if (count >= definition.maxStacks)
            return false;

        ++count;
        RebuildStats();

        return true;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cSpellInstance::Update(float _deltaTime)
    {
        m_cooldownRemaining = std::max(0.0f, m_cooldownRemaining - _deltaTime);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    bool cSpellInstance::IsOnCooldown() const
    {
        return m_cooldownRemaining > 0.0f;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    float cSpellInstance::GetCooldownRemaining() const
    {
        return m_cooldownRemaining;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cSpellInstance::StartCooldown()
    {
        m_cooldownRemaining = m_stats.cooldown;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cSpellInstance::RebuildStats()
    {
        m_stats = m_baseStats;

        for (size_t augmentIndex = 0; augmentIndex < m_augmentCounts.size(); ++augmentIndex)
        {
            const uint8_t count = m_augmentCounts[augmentIndex];

            if (count == 0)
                continue;

            const sSpellStats& modifier = SpellManager::GetAugment(static_cast<sSpellAugment::Enum>(augmentIndex)).statModifier;
            const float multiplier = static_cast<float>(count);

            m_stats.damage              += modifier.damage           * multiplier;
            m_stats.cooldown            += modifier.cooldown         * multiplier;
            m_stats.projectileSpeed     += modifier.projectileSpeed  * multiplier;
            m_stats.duration            += modifier.duration         * multiplier;
            m_stats.projectileRadius    += modifier.projectileRadius * multiplier;
            m_stats.projectileCount     += modifier.projectileCount  * static_cast<int>(count);
        }

        m_stats.cooldown         = std::max(0.05f, m_stats.cooldown);
        m_stats.projectileSpeed  = std::max(0.0f, m_stats.projectileSpeed);
        m_stats.duration         = std::max(0.0f, m_stats.duration);
        m_stats.projectileRadius = std::max(0.0f, m_stats.projectileRadius);
        m_stats.projectileCount  = std::max(1, m_stats.projectileCount);
    }

    // -------------------------------------------------------------------------------------------------------------------------
}

// -------------------------------------------------------------------------------------------------------------------------
