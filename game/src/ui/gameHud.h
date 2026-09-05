#pragma once

#include <array>

namespace UI
{
    struct sDungeonHudState
    {
        bool defeated = false;
        float distance = 0.0f;
        float offsetX = 0.0f;
        float offsetZ = 0.0f;
        float healthFraction = 1.0f;
        bool inArena = false;
    };

    struct sHudState
    {
        std::array<sDungeonHudState, 4> dungeons{};
        float health                = 100.0f;
        float maxHealth             = 100.0f;
        float spellCooldown         = 0.0f;
        float spellCooldownDuration = 1.0f;

        // Display placeholders until mana and progression gameplay are implemented.
        float        mana          = 100.0f;
        float        maxMana       = 100.0f;
        unsigned int xp            = 0;
        unsigned int xpToNextLevel = 100;
        unsigned int level         = 1;
    };

    class cGameHud
    {
        public:

            void Draw(const sHudState& _rState) const;
    };
}
