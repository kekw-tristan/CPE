#pragma once

#include "../item/item.h"

#include <array>
#include <cstdint>

namespace UI
{
    enum class eInventoryAction
    {
        MoveItem,
        EquipArmor,
        EquipUsable,
        EquipSpell,
        UnequipArmor,
        UnequipUsable,
        UnequipSpell
    };

    struct sDungeonHudState
    {
        bool  defeated       = false;
        float distance       = 0.0f;
        float offsetX        = 0.0f;
        float offsetZ        = 0.0f;
        float healthFraction = 1.0f;
        bool  inArena        = false;
    };

    struct sInventorySlotHudState
    {
        Gameplay::sItemId::Enum item = Gameplay::sItemId::Undefined;
        uint32_t amount = 0;
    };

    struct sInventoryHudState
    {
        static constexpr size_t c_numberOfInventorySlots    = 24;
        static constexpr size_t c_numberOfUsableSlots       = 4;
        static constexpr size_t c_numberOfSpellSlots        = 6;

        bool visible = false;

        std::array<sInventorySlotHudState, c_numberOfInventorySlots>                inventorySlots{};
        std::array<sInventorySlotHudState, Gameplay::sArmorSlot::NumberOfElements>  armorSlots{};
        std::array<sInventorySlotHudState, c_numberOfUsableSlots>                   usableSlots{};
        std::array<sInventorySlotHudState, c_numberOfSpellSlots>                    spellSlots{};
    };

    struct sHudState
    {
        std::array<sDungeonHudState, 4> dungeons{};

        float health    = 100.0f;
        float maxHealth = 100.0f;

        float spellCooldown         = 0.0f;
        float spellCooldownDuration = 1.0f;

        float mana      = 100.0f;
        float maxMana   = 100.0f;

        unsigned int xp             = 0;
        unsigned int xpToNextLevel  = 100;
        unsigned int level          = 1;

        sInventoryHudState inventory{};
    };

    class cGameHud
    {
    public:

        void Draw(const sHudState& _rState);

        bool ConsumeInventoryAction(
            eInventoryAction& _rAction,
            size_t& _rSourceSlot,
            size_t& _rDestinationSlot);


    private:

        enum class eInventoryDropTarget
        {
            Inventory,
            Armor,
            Usable,
            Spell
        };

        void DrawInventory(const sInventoryHudState& _rState);
        void AcceptInventorySlotDrop(
            const sInventoryHudState& _rState,
            eInventoryDropTarget _target,
            size_t _destinationSlot);

    private:

        bool                m_hasInventoryAction = false;
        eInventoryAction    m_inventoryAction    = eInventoryAction::MoveItem;
        size_t              m_sourceInventorySlot = 0;
        size_t              m_destinationInventorySlot = 0;
    };
}
