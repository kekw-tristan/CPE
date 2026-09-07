#pragma once

#include "item.h"

#include <array>
#include <cstdint>

namespace Gameplay
{
    class cInventory
    {
        public:

            static constexpr size_t c_numberOfInventorySlots    = 24;
            static constexpr size_t c_numberOfUsableSlots       = 4;
            static constexpr size_t c_numberOfSpellSlots        = 6;

        public:

            using InventorySlots    = std::array<sItemStack, c_numberOfInventorySlots>;
            using ArmorSlots        = std::array<sItemStack, sArmorSlot::NumberOfElements>;
            using UsableSlots       = std::array<sItemStack, c_numberOfUsableSlots>;
            using SpellSlots        = std::array<sItemStack, c_numberOfSpellSlots>;

        public:

            bool AddItem(sItemId::Enum _item, uint32_t _amount = 1);
            bool RemoveItem(sItemId::Enum _item, uint32_t _amount = 1);

            bool MoveItem(size_t _sourceSlot, size_t _destinationSlot);

            bool EquipArmor(size_t _inventorySlot);
            bool UnequipArmor(sArmorSlot::Enum _armorSlot, size_t _inventorySlot);

            bool EquipUsable(size_t _inventorySlot, size_t _usableSlot);
            bool UnequipUsable(size_t _usableSlot, size_t _inventorySlot);

            bool EquipSpell(size_t _inventorySlot, size_t _spellSlot);
            bool UnequipSpell(size_t _spellSlot, size_t _inventorySlot);

            bool UseItem(size_t _usableSlot);


            bool HasItem(sItemId::Enum _item, uint32_t _amount = 1) const;
            uint32_t GetItemCount(sItemId::Enum _item) const;

        public:


            const InventorySlots& GetInventorySlots() const
            {
                return m_inventorySlots;
            }

            const ArmorSlots& GetArmorSlots() const
            {
                return m_armorSlots;
            }

            const UsableSlots& GetUsableSlots() const
            {
                return m_usableSlots;
            }

            const SpellSlots& GetSpellSlots() const
            {
                return m_spellSlots;
            }


        private:

            InventorySlots  m_inventorySlots{};
            ArmorSlots      m_armorSlots{};
            UsableSlots     m_usableSlots{};
            SpellSlots      m_spellSlots{};
    };

}
