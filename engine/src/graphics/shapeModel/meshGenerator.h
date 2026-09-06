#pragma once

#include "graphics/bounds.h"
#include "graphics/meshData.h"
#include "graphics/vertex.h"
#include "math/vector2.h"

#include <cstdint>
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

    struct sBeveledCubeDesc
    {
        float width     = 1.0f;
        float height    = 1.0f;
        float depth     = 1.0f;
        float bevel     = 0.1f; // Chamfer width; strictly less than half the smallest dimension.
    };

    struct sFrustumDesc
    {
        float bottomRadius  = 0.5f;
        float topRadius     = 0.25f; // Either radius may be zero, but not both.
        float height        = 1.0f;
        int segments        = 32;
    };

    struct sWedgeDesc
    {
        float width     = 1.0f; // Ramp rises from -X to +X, extruded along Z.
        float height    = 1.0f;
        float depth     = 1.0f;
    };

    struct sTriangularPrismDesc
    {
        float width     = 1.0f; // Isosceles XY triangle with its apex along +Y.
        float height    = 1.0f;
        float depth     = 1.0f;
    };

    struct sIcoSphereDesc
    {
        float   radius       = 0.5f;
        int     subdivisions = 2; // 0..6; each subdivision quadruples the triangle count.
    };

    struct sRockDesc
    {
        float    radius         = 0.5f;
        float    roughness      = 0.2f; // Radial displacement fraction in [0, 1).
        int      subdivisions   = 1;
        uint32_t seed           = 1;
    };

    struct sGrassBladeDesc
    {
        float width         = 0.15f;
        float height        = 1.0f; // Root at the origin, grows along +Y.
        float bend          = 0.2f; // Signed tip displacement along Z.
        int   segments      = 4;
        bool  doubleSided   = true;
    };

    struct sCapsuleDesc
    {
        float   radius      = 0.25f;
        float   height      = 1.0f;    // Total Y extent, including hemispheres; at least 2 * radius.
        int     segments    = 32;
        int     rings       = 8;       // Rings per hemisphere.
    };

    struct sArchDesc
    {
        float innerRadius   = 0.3f;
        float outerRadius   = 0.5f;
        float depth         = 0.25f;        // XY annular sector extruded along Z.
        float startAngle    = 0.0f;         // Radians, counter-clockwise from +X.
        float endAngle      = 3.1415927f;   // Positive sweep strictly less than a full turn.
        int   segments      = 24;
    };

    struct sExtrudedPolygonDesc
    {
        // Simple XY boundary in either winding, concave or convex, without holes.
        // An optional repeated closing point and collinear boundary points are accepted.
        std::vector<Math::cVec2f> points    = { { -0.5f, -0.5f }, { 0.5f, -0.5f }, { 0.5f, 0.5f }, { -0.5f, 0.5f } };
        float depth                         = 1.0f; // Front/back at +/- depth / 2 along Z.
    };

    struct sDiscDesc
    {
        float radius    = 0.5f; // XZ plane, normal +Y.
        int   segments  = 32;
    };

    struct sArcDesc
    {
        float innerRadius   = 0.3f;         // XZ plane, normal +Y; zero produces a sector.
        float outerRadius   = 0.5f;
        float startAngle    = 0.0f;         // Radians from +Z toward +X.
        float endAngle      = 3.1415927f;   // Positive sweep up to a full turn.
        int segments        = 24;
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

            // New descriptors reject invalid/non-finite parameters with std::invalid_argument.
            // Segment counts are limited to 4096; capsule hemisphere rings to 256.
            static sMeshData CreateBeveledCube(const sBeveledCubeDesc& _rDesc);
            static sMeshData CreateFrustum(const sFrustumDesc& _rDesc);
            static sMeshData CreateWedge(const sWedgeDesc& _rDesc);
            static sMeshData CreateTriangularPrism(const sTriangularPrismDesc& _rDesc);
            static sMeshData CreateIcoSphere(const sIcoSphereDesc& _rDesc);
            static sMeshData CreateRock(const sRockDesc& _rDesc);
            static sMeshData CreateGrassBlade(const sGrassBladeDesc& _rDesc);
            static sMeshData CreateCapsule(const sCapsuleDesc& _rDesc);
            static sMeshData CreateArch(const sArchDesc& _rDesc);
            static sMeshData CreateExtrudedPolygon(const sExtrudedPolygonDesc& _rDesc);
            static sMeshData CreateDisc(const sDiscDesc& _rDesc);
            static sMeshData CreateArc(const sArcDesc& _rDesc);

        private:

            static sBounds CalculateBounds(const std::vector<sVertex>& _rVertices);
    };
}
