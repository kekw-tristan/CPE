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
            BeveledCube,
            Frustum,
            Wedge,
            TriangularPrism,
            IcoSphere,
            Rock,
            GrassBlade,
            Capsule,
            Arch,
            ExtrudedPolygon,
            Disc,
            Arc,

            NumberOfElements,

            Undefined = -1
        };
    };
}