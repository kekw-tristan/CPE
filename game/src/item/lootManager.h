#pragma once

#include "item.h"

#include "math/vector3.h"
#include "../world/enemy/enemySpawn.h"

#include <random>
#include <vector>

namespace Gameplay
{
    class cInventory;

    struct sLootDrop
    {
        sItemStack item;
        Engine::Math::cVec3f position;
    };

    class cLootManager
    {
        public:

            void DropEnemyLoot(const Engine::Math::cVec3f& _rPosition, bool _isBoss, World::sBossId::Enum _bossId);
            void CollectNearby(const Engine::Math::cVec3f& _rPlayerPosition, cInventory& _rInventory);
            void Clear();

            const std::vector<sItemStack>& GetCollectedItems() const;
            void ClearCollectedItems();
            const std::vector<sLootDrop>& GetDrops() const;
            uint64_t GetRevision() const;

        private:

            void Drop(const Engine::Math::cVec3f& _rPosition, const sItemStack& _rItem);
            sItemStack RollRegularDrop();
            sItemStack RollArmor();

        private:

            std::mt19937            m_random{ 1337 };
            std::vector<sLootDrop>  m_drops;
            std::vector<sItemStack> m_collectedItems;
            uint64_t                m_revision = 1;

            static constexpr float c_pickupRadius = 2.0f;
    };
}
