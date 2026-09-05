#pragma once

#include "graphics/bounds.h"
#include "graphics/meshData.h"
#include "graphics/vertex.h"

#include <vector>

namespace Engine::GFX
{
    struct sPlaneDesc
    {
        float width = 1.0f;
        float depth = 1.0f;

        int segmentsX = 1; 
        int segmentsZ = 1;
    };

    struct sCubeDesc
    {
        float width = 1.0f;
        float height = 1.0f;
        float depth = 1.0f;
    };

    struct sPyramidDesc
    {
        float width = 1.0f;
        float height = 1.0f;
        float depth = 1.0f;
    };

    struct sSphereDesc
    {
        float radius = 0.5f;

        int segments = 32;
        int rings = 16;
    };

    struct sCylinderDesc
    {
        float radius = 0.5f;
        float height = 1.0f;

        int segments = 32;
    };

    struct sConeDesc
    {
        float radius = 0.5f;
        float height = 1.0f;

        int segments = 32;
    };

    struct sTorusDesc
    {
        float majorRadius   = 0.375f;
        float tubeRadius    = 0.125f;
        int   segments      = 24;
        int   tubeSegments  = 12;
    };

    struct sCrystalDesc
    {
        float radius    = 0.5f;
        float height    = 1.0f;
        int   segments  = 6;
    };

    class cMeshGenerator
    {

        public:

            static sMeshData CreatePlane(const sPlaneDesc& _rDesc);
            static sMeshData CreateCube(const sCubeDesc& _rDesc);
            static sMeshData CreatePyramid(const sPyramidDesc& _rDesc);
            static sMeshData CreateSphere(const sSphereDesc& _rDesc);
            static sMeshData CreateCylinder(const sCylinderDesc& _rDesc);
            static sMeshData CreateCone(const sConeDesc& _rDesc);
            static sMeshData CreateTorus(const sTorusDesc& _rDesc);
            static sMeshData CreateCrystal(const sCrystalDesc& _rDesc);

        private:

            static sBounds CalculateBounds(const std::vector<sVertex>& _rVertices);
    };
}