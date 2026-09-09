#include "lootManager.h"

#include "inventory.h"
#include "itemDatabase.h"

#include "../spells/spellManager.h"

#include <array>
#include <cmath>

namespace Gameplay
{
    void cLootManager::DropEnemyLoot(const Engine::Math::cVec3f& _rPosition, bool _isBoss, World::sBossId::Enum _bossId)
    {
        if (_isBoss && _bossId != World::sBossId::Undefined)
        {
            const sSpellId::Enum spellId = SpellManager::GetBoss(_bossId).spellReward;
            if (spellId != sSpellId::Undefined)
                Drop(_rPosition, { SpellManager::GetSpell(spellId).inventoryItem, 1 });
        }

        Drop(_rPosition, RollRegularDrop());
    }

    // ---------------------------------------------------------------------------------------------------------------------

    void cLootManager::CollectNearby(const Engine::Math::cVec3f& _rPlayerPosition, cInventory& _rInventory)
    {
        const float pickupRadiusSquared = c_pickupRadius * c_pickupRadius;

        for (size_t index = 0; index < m_drops.size();)
        {
            const Engine::Math::cVec3f offset = m_drops[index].position - _rPlayerPosition;
            if (offset.dot(offset) > pickupRadiusSquared || !_rInventory.AddItem(m_drops[index].item))
            {
                ++index;
                continue;
            }

            m_collectedItems.push_back(m_drops[index].item);
            m_drops[index] = m_drops.back();
            m_drops.pop_back();
            ++m_revision;
        }
    }

    // ---------------------------------------------------------------------------------------------------------------------

    void cLootManager::Clear()
    {
        m_drops.clear();
        m_collectedItems.clear();
        ++m_revision;
    }

    // ---------------------------------------------------------------------------------------------------------------------

    const std::vector<sItemStack>& cLootManager::GetCollectedItems() const
    {
        return m_collectedItems;
    }

    // ---------------------------------------------------------------------------------------------------------------------

    void cLootManager::ClearCollectedItems()
    {
        m_collectedItems.clear();
    }

    // ---------------------------------------------------------------------------------------------------------------------

    const std::vector<sLootDrop>& cLootManager::GetDrops() const
    {
        return m_drops;
    }

    // ---------------------------------------------------------------------------------------------------------------------

    uint64_t cLootManager::GetRevision() const
    {
        return m_revision;
    }

    // ---------------------------------------------------------------------------------------------------------------------

    void cLootManager::Drop(const Engine::Math::cVec3f& _rPosition, const sItemStack& _rItem)
    {
        if (!_rItem.IsEmpty())
        {
            m_drops.push_back({ _rItem, _rPosition });
            ++m_revision;
        }
    }

    // ---------------------------------------------------------------------------------------------------------------------

    sItemStack cLootManager::RollRegularDrop()
    {
        std::discrete_distribution<int> dropType({ 40, 30, 30 });

        switch (dropType(m_random))
        {
            case 0:
                return { sItemId::HealthPotion, 1 };

            case 1:
                return { sItemId::ManaPotion, 1 };

            default:
                return RollArmor();
        }
    }

    // ---------------------------------------------------------------------------------------------------------------------

    sItemStack cLootManager::RollArmor()
    {
        constexpr std::array<sItemId::Enum, sArmorSlot::NumberOfElements> c_armorItems =
        {
            sItemId::ForestHelmet,
            sItemId::ForestChest,
            sItemId::ForestRing,
            sItemId::ForestLegs,
            sItemId::ForestBoots
        };

        std::uniform_int_distribution<size_t> armorItem(0, c_armorItems.size() - 1);
        std::discrete_distribution<int> rarity({ 70, 25, 5 });

        sItemStack result{};
        result.item   = c_armorItems[armorItem(m_random)];
        result.amount = 1;
        result.rarity = static_cast<sItemRarity::Enum>(rarity(m_random));

        const uint32_t baseArmor = GetItemDefinition(result.item).baseArmor;
        switch (result.rarity)
        {
            case sItemRarity::Common:
                result.armor = baseArmor;
                break;

            case sItemRarity::Rare:
                result.armor = static_cast<uint32_t>(std::ceil(static_cast<float>(baseArmor) * 1.5f));
                break;

            case sItemRarity::Legendary:
                result.armor = static_cast<uint32_t>(std::ceil(static_cast<float>(baseArmor) * 2.25f));
                break;

            default:
                break;
        }

        return result;
    }
}
