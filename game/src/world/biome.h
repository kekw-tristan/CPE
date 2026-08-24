#pragma once

namespace World
{
    struct sBiomeType
    {
        enum Enum
        {
            Forest, 
            Swamp, 
            Ice, 
            Lava,

            NumberOfElements,
            Undefined = -1
        };
    };
}