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

    struct sEnemyType
    {
        enum Enum 
        {
            ForestCrawler,
            ForestBrute,
            ForestThornwolf,
            ForestSporecap,

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
    };

}
