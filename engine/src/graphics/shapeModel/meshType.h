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
            Torus,
            Crystal,

            NumberOfElements,

            Undefined = -1
        };
    };
}