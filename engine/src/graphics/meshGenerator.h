#pragma once

#include "graphics/bounds.h"
#include "graphics/meshData.h"
#include "graphics/vertex.h"

#include <vector>

namespace Engine::GFX
{

    struct sMeshTypes
    {
        enum Enum
        {
            Cube,
            Pyramid,
            NumberOfElements,
            Undefined = -1
        };
    };

    struct sCubeDesc
    {
        float width     = 1.0f;
        float height    = 1.0f;
        float depth     = 1.0f;
    };

    struct sPyramidDesc
    {
        int baseCornerCount     = 4;

        float baseRadius        = 0.5f;
        float height            = 1.0f;
        float rotationRadians   = 0.0f;
    };

    class cMeshGenerator
    {

        public:

            static sMeshData CreateCube(const sCubeDesc& _rDesc);
            static sMeshData CreatePyramid(const sPyramidDesc& _rDesc);

        private:

            static sBounds CalculateBounds(const std::vector<sVertex>& _rVertices);
    };

}