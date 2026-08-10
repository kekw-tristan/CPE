#pragma once

namespace Engine::GFX
{
    struct sMeshTypes
    {
        enum Enum
        {
            Plane,
            Cube,
            Pyramid,
            Sphere,
            Cylinder,
            Cone,

            NumberOfElements,

            Undefined = -1
        };
    };
}