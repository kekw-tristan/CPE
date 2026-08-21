#pragma once

namespace Engine::GFX
{
    struct sMeshTypes
    {
        enum Enum
        {
            Plane,
            ChunkPlane,
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