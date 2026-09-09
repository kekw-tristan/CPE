#include "inventory.h"

#include <algorithm>

#include "itemDatabase.h"

namespace Gameplay
{
    namespace
    {
        bool AreStacksCompatible(const sItemStack& _rLeft, const sItemStack& _rRight)
        {
            return _rLeft.item == _rRight.item
                && _rLeft.rarity == _rRight.rarity
                && _rLeft.armor == _rRight.armor;
        }

        bool CanStoreItem(
            const cInventory::InventorySlots& _rSlots,
            const sItemStack& _rItem,
            uint32_t _maxStack,
            uint32_t _amount)
        {
            uint64_t availableCapacity = 0;

            for (const sItemStack& slot : _rSlots)
            {
                if (AreStacksCompatible(slot, _rItem) && slot.amount < _maxStack)
                    availableCapacity += _maxStack - slot.amount;
                else if (slot.IsEmpty())
                    availableCapacity += _maxStack;

                if (availableCapacity >= _amount)
                    return true;
            }

            return false;
        }
    }

    // ---------------------------------------------------------------------------------------------------------------------

    bool cInventory::AddItem(sItemId::Enum _item, uint32_t _amount)
    {
        return AddItem({ _item, _amount });
    }

    // ---------------------------------------------------------------------------------------------------------------------

    bool cInventory::AddItem(const sItemStack& _rItem)
    {
        if (_rItem.item == sItemId::Undefined || _rItem.amount == 0)
            return false;

        const sItemDefinition& definition = GetItemDefinition(_rItem.item);
        const uint32_t maxStack = std::max(1u, definition.maxStack);
        uint32_t amount = _rItem.amount;

        if (!CanStoreItem(m_inventorySlots, _rItem, maxStack, amount))
            return false;

        // Fill existing stacks first.
        for (sItemStack& slot : m_inventorySlots)
        {
            if (!AreStacksCompatible(slot, _rItem) || slot.amount >= maxStack)
                continue;

            const uint32_t available = maxStack - slot.amount;
            const uint32_t amountToAdd = std::min(available, amount);

            slot.amount += amountToAdd;
            amount -= amountToAdd;

            if (amount == 0)
                return true;
        }

        // Create new stacks.
        for (sItemStack& slot : m_inventorySlots)
        {
            if (!slot.IsEmpty())
                continue;

            const uint32_t amountToAdd = std::min(maxStack, amount);

            slot = _rItem;
            slot.amount = amountToAdd;

            amount -= amountToAdd;

            if (amount == 0)
                return true;
        }

        return true;
    }

    // ---------------------------------------------------------------------------------------------------------------------

    bool cInventory::RemoveItem(sItemId::Enum _item, uint32_t _amount)
    {
        if (_item == sItemId::Undefined || _amount == 0)
            return false;

        if (!HasItem(_item, _amount))
            return false;

        auto removeFromSlots = [&_item, &_amount](auto& _rSlots)
        {
            for (sItemStack& slot : _rSlots)
            {
                if (slot.item != _item)
                    continue;

                const uint32_t amountToRemove = std::min(slot.amount, _amount);

                slot.amount -= amountToRemove;
                _amount -= amountToRemove;

                if (slot.amount == 0)
                    slot = {};

                if (_amount == 0)
                    return;
            }
        };

        removeFromSlots(m_inventorySlots);

        if (_amount > 0)
            removeFromSlots(m_usableSlots);

        if (_amount > 0)
            removeFromSlots(m_spellSlots);

        if (_amount > 0)
            removeFromSlots(m_armorSlots);

        return _amount == 0;
    }

    // ---------------------------------------------------------------------------------------------------------------------

    bool cInventory::MoveItem(size_t _sourceSlot, size_t _destinationSlot)
    {
        if (_sourceSlot >= m_inventorySlots.size() || _destinationSlot >= m_inventorySlots.size())
            return false;

        if (_sourceSlot == _destinationSlot)
            return true;

        sItemStack& source = m_inventorySlots[_sourceSlot];
        sItemStack& destination = m_inventorySlots[_destinationSlot];

        if (source.IsEmpty())
            return false;

        if (destination.IsEmpty())
        {
            destination = source;
            source = {};

            return true;
        }

        if (AreStacksCompatible(source, destination))
        {
            const sItemDefinition& definition = GetItemDefinition(source.item);
            const uint32_t maxStack = std::max(1u, definition.maxStack);

            if (destination.amount < maxStack)
            {
                const uint32_t available = maxStack - destination.amount;
                const uint32_t amountToMove = std::min(source.amount, available);

                destination.amount += amountToMove;
                source.amount -= amountToMove;

                if (source.amount == 0)
                    source = {};

                return true;
            }
        }

        std::swap(source, destination);

        return true;
    }

    // ---------------------------------------------------------------------------------------------------------------------

    bool cInventory::EquipArmor(size_t _inventorySlot)
    {
        if (_inventorySlot >= m_inventorySlots.size())
            return false;

        sItemStack& inventorySlot = m_inventorySlots[_inventorySlot];

        if (inventorySlot.IsEmpty())
            return false;

        const sItemDefinition& definition = GetItemDefinition(inventorySlot.item);

        if (definition.type != sItemType::Armor || definition.armorSlot == sArmorSlot::Undefined)
            return false;

        const size_t armorSlotIndex = static_cast<size_t>(definition.armorSlot);

        if (armorSlotIndex >= m_armorSlots.size())
            return false;

        sItemStack& armorSlot = m_armorSlots[armorSlotIndex];

        // Nothing equipped yet.
        if (armorSlot.IsEmpty())
        {
            armorSlot = inventorySlot;
            inventorySlot = {};

            return true;
        }

        // Replace currently equipped armor with the new item.
        std::swap(armorSlot, inventorySlot);

        return true;
    }

    // ---------------------------------------------------------------------------------------------------------------------

    bool cInventory::UnequipArmor(sArmorSlot::Enum _armorSlot, size_t _inventorySlot)
    {
        if (_armorSlot == sArmorSlot::Undefined || _inventorySlot >= m_inventorySlots.size())
            return false;

        const size_t armorSlotIndex = static_cast<size_t>(_armorSlot);

        if (armorSlotIndex >= m_armorSlots.size())
            return false;

        sItemStack& armorSlot = m_armorSlots[armorSlotIndex];

        if (armorSlot.IsEmpty())
            return false;

        sItemStack& inventorySlot = m_inventorySlots[_inventorySlot];

        if (!inventorySlot.IsEmpty())
            return false;

        inventorySlot = armorSlot;
        armorSlot = {};

        return true;
    }

    // ---------------------------------------------------------------------------------------------------------------------

    bool cInventory::EquipUsable(size_t _inventorySlot, size_t _usableSlot)
    {
        if (_inventorySlot >= m_inventorySlots.size() || _usableSlot >= m_usableSlots.size())
            return false;

        sItemStack& inventorySlot = m_inventorySlots[_inventorySlot];

        if (inventorySlot.IsEmpty())
            return false;

        const sItemDefinition& definition = GetItemDefinition(inventorySlot.item);

        if (definition.type != sItemType::Usable)
            return false;

        sItemStack& usableSlot = m_usableSlots[_usableSlot];

        if (usableSlot.IsEmpty())
        {
            usableSlot = inventorySlot;
            inventorySlot = {};

            return true;
        }

        if (AreStacksCompatible(usableSlot, inventorySlot))
        {
            const uint32_t maxStack = std::max(1u, definition.maxStack);

            if (usableSlot.amount < maxStack)
            {
                const uint32_t available = maxStack - usableSlot.amount;
                const uint32_t amountToMove = std::min(inventorySlot.amount, available);

                usableSlot.amount += amountToMove;
                inventorySlot.amount -= amountToMove;

                if (inventorySlot.amount == 0)
                    inventorySlot = {};

                return true;
            }
        }

        std::swap(usableSlot, inventorySlot);

        return true;
    }

    // ---------------------------------------------------------------------------------------------------------------------

    bool cInventory::UnequipUsable(size_t _usableSlot, size_t _inventorySlot)
    {
        if (_usableSlot >= m_usableSlots.size() || _inventorySlot >= m_inventorySlots.size())
            return false;

        sItemStack& usableSlot = m_usableSlots[_usableSlot];

        if (usableSlot.IsEmpty())
            return false;

        const sItemDefinition& definition = GetItemDefinition(usableSlot.item);
        const uint32_t maxStack = std::max(1u, definition.maxStack);

        sItemStack& inventorySlot = m_inventorySlots[_inventorySlot];

        if (inventorySlot.IsEmpty())
        {
            inventorySlot = usableSlot;
            usableSlot = {};

            return true;
        }

        if (!AreStacksCompatible(inventorySlot, usableSlot) || inventorySlot.amount >= maxStack)
            return false;

        const uint32_t available = maxStack - inventorySlot.amount;
        const uint32_t amountToMove = std::min(usableSlot.amount, available);

        inventorySlot.amount += amountToMove;
        usableSlot.amount -= amountToMove;

        if (usableSlot.amount == 0)
            usableSlot = {};

        return true;
    }

    // ---------------------------------------------------------------------------------------------------------------------

    bool cInventory::MoveUsable(size_t _sourceSlot, size_t _destinationSlot)
    {
        if (_sourceSlot >= m_usableSlots.size() || _destinationSlot >= m_usableSlots.size())
            return false;

        if (_sourceSlot == _destinationSlot)
            return true;

        sItemStack& source = m_usableSlots[_sourceSlot];

        if (source.IsEmpty())
            return false;

        std::swap(source, m_usableSlots[_destinationSlot]);

        return true;
    }

    // ---------------------------------------------------------------------------------------------------------------------

    bool cInventory::EquipSpell(size_t _inventorySlot, size_t _spellSlot)
    {
        if (_inventorySlot >= m_inventorySlots.size() || _spellSlot >= m_spellSlots.size())
            return false;

        sItemStack& inventorySlot = m_inventorySlots[_inventorySlot];

        if (inventorySlot.IsEmpty())
            return false;

        const sItemDefinition& definition = GetItemDefinition(inventorySlot.item);

        if (definition.type != sItemType::Spell)
            return false;

        sItemStack& spellSlot = m_spellSlots[_spellSlot];

        if (spellSlot.IsEmpty())
        {
            spellSlot = inventorySlot;
            inventorySlot = {};

            return true;
        }

        if (AreStacksCompatible(spellSlot, inventorySlot))
        {
            const uint32_t maxStack = std::max(1u, definition.maxStack);

            if (spellSlot.amount < maxStack)
            {
                const uint32_t available = maxStack - spellSlot.amount;
                const uint32_t amountToMove = std::min(inventorySlot.amount, available);

                spellSlot.amount += amountToMove;
                inventorySlot.amount -= amountToMove;

                if (inventorySlot.amount == 0)
                    inventorySlot = {};

                return true;
            }
        }

        std::swap(spellSlot, inventorySlot);

        return true;
    }

    // ---------------------------------------------------------------------------------------------------------------------

    bool cInventory::UnequipSpell(size_t _spellSlot, size_t _inventorySlot)
    {
        if (_spellSlot >= m_spellSlots.size() || _inventorySlot >= m_inventorySlots.size())
            return false;

        sItemStack& spellSlot = m_spellSlots[_spellSlot];

        if (spellSlot.IsEmpty())
            return false;

        const sItemDefinition& definition = GetItemDefinition(spellSlot.item);
        const uint32_t maxStack = std::max(1u, definition.maxStack);

        sItemStack& inventorySlot = m_inventorySlots[_inventorySlot];

        if (inventorySlot.IsEmpty())
        {
            inventorySlot = spellSlot;
            spellSlot = {};

            return true;
        }

        if (!AreStacksCompatible(inventorySlot, spellSlot) || inventorySlot.amount >= maxStack)
            return false;

        const uint32_t available = maxStack - inventorySlot.amount;
        const uint32_t amountToMove = std::min(spellSlot.amount, available);

        inventorySlot.amount += amountToMove;
        spellSlot.amount -= amountToMove;

        if (spellSlot.amount == 0)
            spellSlot = {};

        return true;
    }

    // ---------------------------------------------------------------------------------------------------------------------

    bool cInventory::MoveSpell(size_t _sourceSlot, size_t _destinationSlot)
    {
        if (_sourceSlot >= m_spellSlots.size() || _destinationSlot >= m_spellSlots.size())
            return false;

        if (_sourceSlot == _destinationSlot)
            return true;

        sItemStack& source = m_spellSlots[_sourceSlot];

        if (source.IsEmpty())
            return false;

        std::swap(source, m_spellSlots[_destinationSlot]);

        return true;
    }

    // ---------------------------------------------------------------------------------------------------------------------

    void cInventory::ClearSpells()
    {
        const auto clearSpellItems = [](auto& _rSlots)
        {
            for (sItemStack& slot : _rSlots)
            {
                if (!slot.IsEmpty() && GetItemDefinition(slot.item).type == sItemType::Spell)
                    slot = {};
            }
        };

        clearSpellItems(m_inventorySlots);
        clearSpellItems(m_spellSlots);
    }

    // ---------------------------------------------------------------------------------------------------------------------

    bool cInventory::UseItem(size_t _usableSlot)
    {
        if (_usableSlot >= m_usableSlots.size())
            return false;

        sItemStack& usableSlot = m_usableSlots[_usableSlot];

        if (usableSlot.IsEmpty())
            return false;

        const sItemDefinition& definition = GetItemDefinition(usableSlot.item);

        if (definition.type != sItemType::Usable)
            return false;

        --usableSlot.amount;

        if (usableSlot.amount == 0)
            usableSlot = {};

        return true;
    }

    // ---------------------------------------------------------------------------------------------------------------------

    bool cInventory::HasItem(sItemId::Enum _item, uint32_t _amount) const
    {
        if (_item == sItemId::Undefined || _amount == 0)
            return false;

        return GetItemCount(_item) >= _amount;
    }

    // ---------------------------------------------------------------------------------------------------------------------

    uint32_t cInventory::GetItemCount(sItemId::Enum _item) const
    {
        if (_item == sItemId::Undefined)
            return 0;

        uint32_t count = 0;

        for (const sItemStack& slot : m_inventorySlots)
        {
            if (slot.item == _item)
                count += slot.amount;
        }

        for (const sItemStack& slot : m_usableSlots)
        {
            if (slot.item == _item)
                count += slot.amount;
        }

        for (const sItemStack& slot : m_spellSlots)
        {
            if (slot.item == _item)
                count += slot.amount;
        }

        for (const sItemStack& slot : m_armorSlots)
        {
            if (slot.item == _item)
                count += slot.amount;
        }

        return count;
    }

    // ---------------------------------------------------------------------------------------------------------------------

    uint32_t cInventory::GetArmor() const
    {
        uint32_t armor = 0;

        for (const sItemStack& slot : m_armorSlots)
            armor += slot.armor;

        return armor;
    }
}
