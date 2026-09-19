#pragma once

#include "math/vector3.h"

#include <cstdint>

namespace World
{

    struct sBossId
    {
        enum Enum
        {
            ForestCrawler,
            ForestBrute,
            ForestThornwolf,
            ForestSporecap,

            DesertLancer,
            DesertSentinel,
            DesertStalker,
            DesertScorpion,

            IceWraith,
            IceGolem,
            IceHarpy,
            IceTitan,

            LavaImp,
            LavaGolem,
            LavaWyrm,
            LavaTitan,

            NumberOfElements,
            Undefined = -1
        };
    };

    inline float GetBossArenaHalfExtent(sBossId::Enum _bossId)
    {
        if (_bossId == sBossId::ForestCrawler)
            return 18.0f;
        if (_bossId == sBossId::ForestThornwolf)
            return 20.0f;
        return _bossId >= sBossId::ForestCrawler && _bossId <= sBossId::ForestSporecap ? 36.0f : 12.5f;
    }

    inline float GetBossArenaHeight(sBossId::Enum _bossId)
    {
        if (_bossId == sBossId::ForestCrawler)
            return 64.0f;
        if (_bossId == sBossId::ForestSporecap)
            return 134.0f;
        if (_bossId == sBossId::ForestThornwolf)
            return 184.0f;
        return _bossId == sBossId::ForestBrute ? 224.0f : 0.0f;
    }

    inline bool IsInsideBossArena(sBossId::Enum _bossId,
        const Engine::Math::cVec3f& _rPosition, const Engine::Math::cVec3f& _rCenter)
    {
        const auto offset = _rPosition - _rCenter;
        const float halfExtent = GetBossArenaHalfExtent(_bossId);
        if (_bossId >= sBossId::ForestCrawler && _bossId <= sBossId::ForestSporecap)
        {
            return offset.x() * offset.x() + offset.z() * offset.z() <= halfExtent * halfExtent
                && offset.y() >= -6.0f && offset.y() <= 6.0f;
        }

        return offset.x() >= -halfExtent && offset.x() <= halfExtent
            && offset.z() >= -halfExtent && offset.z() <= halfExtent
            && offset.y() >= -6.0f && offset.y() <= 6.0f;
    }

    struct sEnemyType
    {
        enum Enum 
        {
            ForestCrawler,
            ForestBrute,
            ForestThornwolf,
            ForestSporecap,
            ForestThornshooter,
            ForestRootcharger,
            ForestBarkguard,

            NumberOfElements,
            Undefined = -1
        };
    };

    struct sEnemyTier
    {
        enum Enum
        {
            Normal,
            Blue,
            Yellow,
            Unique,

            NumberOfElements,
            Undefined = -1
        };
    };

    struct sEnemySpawn
    {
        sEnemyType::Enum     type     = sEnemyType::Undefined;
        Engine::Math::cVec3f position = { 0.f, 0.f, 0.f };
        float                rotation = 0.0f;
        bool                 isBoss = false;
        sBossId::Enum        bossId = sBossId::Undefined;
        sEnemyTier::Enum     tier = sEnemyTier::Normal;
    };

}
