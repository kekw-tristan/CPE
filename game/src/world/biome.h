#pragma once

namespace World
{
    struct sBiomeType
    {
        enum Enum
        {
            Forest, 
            Desert,
            Ice, 
            Lava,

            NumberOfElements,
            Undefined = -1
        };
    };
}