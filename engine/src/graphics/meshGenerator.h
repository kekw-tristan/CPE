#pragma once 

#include "graphics/bounds.h"
#include "graphics/meshData.h"
#include "graphics/vertex.h"

#include <array>

namespace Engine::GFX
{
    struct sMeshTypes
    {
        enum Enum
        {
            Plane, 
            Cube,
            UVSphere,
            Cylinder,
            Cone,
            NumberOfElements,
            Undefined = -1
        };
    };

    struct sCubeDesc
    {
        float width     = 1.f;
        float height    = 1.f;
        float depth     = 1.f; 
        std::array<float, 4> color = {1.f, 1.f, 1.f, 1.f};
    };

    class cMeshGenerator
    {
        public:

            static sMeshData CreateCube(const sCubeDesc& _rDesc);

        private:

            static sBounds CalculateBounds(const std::vector<sVertex>& _rVertices);
    };
}