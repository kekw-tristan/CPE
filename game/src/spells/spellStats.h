#pragma once

namespace Gameplay
{
    struct sSpellStats
    {
        float damage            = 0.0f; 
        float cooldown          = 1.0f; 
        float projectileSpeed   = 10.0f; 
        float duration          = 2.5f;
        float projectileRadius  = 0.8f;
        int   projectileCount   = 1;
        int   pierceCount       = 0;
    };
}

