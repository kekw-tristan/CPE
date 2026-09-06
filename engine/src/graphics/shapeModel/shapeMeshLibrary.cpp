#include "shapeMeshLibrary.h"

#include "graphics/shapeModel/meshGenerator.h"

#include <array>
#include <cassert>
#include <cstddef>

// -------------------------------------------------------------------------------------------------------------------------

namespace Engine::GFX
{

    // -------------------------------------------------------------------------------------------------------------------------

    namespace
    {
        constexpr size_t c_shapeMeshCount = static_cast<size_t>(sMeshTypes::NumberOfElements);

        // -------------------------------------------------------------------------------------------------------------------------

        std::array<sMeshData, c_shapeMeshCount> CreateShapeMeshes()
        {
            std::array<sMeshData, c_shapeMeshCount> meshes;

            meshes[static_cast<size_t>(sMeshTypes::Plane)]      = cMeshGenerator::CreatePlane(sPlaneDesc{});
            meshes[static_cast<size_t>(sMeshTypes::ChunkPlane)] = cMeshGenerator::CreatePlane(sPlaneDesc{32.f, 32.f, 32, 32});
            meshes[static_cast<size_t>(sMeshTypes::Cube)]       = cMeshGenerator::CreateCube(sCubeDesc{});
            meshes[static_cast<size_t>(sMeshTypes::Pyramid)]    = cMeshGenerator::CreatePyramid(sPyramidDesc{});
            meshes[static_cast<size_t>(sMeshTypes::Sphere)]     = cMeshGenerator::CreateSphere(sSphereDesc{});
            meshes[static_cast<size_t>(sMeshTypes::Cylinder)]   = cMeshGenerator::CreateCylinder(sCylinderDesc{});
            meshes[static_cast<size_t>(sMeshTypes::Cone)]       = cMeshGenerator::CreateCone(sConeDesc{});

            meshes[static_cast<size_t>(sMeshTypes::Torus)]      = cMeshGenerator::CreateTorus(sTorusDesc{});
            meshes[static_cast<size_t>(sMeshTypes::Crystal)]    = cMeshGenerator::CreateCrystal(sCrystalDesc{});

            meshes[static_cast<size_t>(sMeshTypes::BeveledCube)] = cMeshGenerator::CreateBeveledCube(sBeveledCubeDesc{});
            meshes[static_cast<size_t>(sMeshTypes::Frustum)] = cMeshGenerator::CreateFrustum(sFrustumDesc{});
            meshes[static_cast<size_t>(sMeshTypes::Wedge)] = cMeshGenerator::CreateWedge(sWedgeDesc{});
            meshes[static_cast<size_t>(sMeshTypes::TriangularPrism)] = cMeshGenerator::CreateTriangularPrism(sTriangularPrismDesc{});
            meshes[static_cast<size_t>(sMeshTypes::IcoSphere)] = cMeshGenerator::CreateIcoSphere(sIcoSphereDesc{});
            meshes[static_cast<size_t>(sMeshTypes::Rock)] = cMeshGenerator::CreateRock(sRockDesc{});
            meshes[static_cast<size_t>(sMeshTypes::GrassBlade)] = cMeshGenerator::CreateGrassBlade(sGrassBladeDesc{});
            meshes[static_cast<size_t>(sMeshTypes::Capsule)] = cMeshGenerator::CreateCapsule(sCapsuleDesc{});
            meshes[static_cast<size_t>(sMeshTypes::Arch)] = cMeshGenerator::CreateArch(sArchDesc{});
            meshes[static_cast<size_t>(sMeshTypes::ExtrudedPolygon)] = cMeshGenerator::CreateExtrudedPolygon(sExtrudedPolygonDesc{});
            meshes[static_cast<size_t>(sMeshTypes::Disc)] = cMeshGenerator::CreateDisc(sDiscDesc{});
            meshes[static_cast<size_t>(sMeshTypes::Arc)] = cMeshGenerator::CreateArc(sArcDesc{});

            return meshes;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        std::array<sMeshData, c_shapeMeshCount>& GetShapeMeshes()
        {
            static std::array<sMeshData, c_shapeMeshCount> s_shapeMeshes = CreateShapeMeshes();
            return s_shapeMeshes;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        size_t GetMeshIndex(sMeshTypes::Enum _meshType)
        {
            assert(_meshType >= 0 && _meshType < sMeshTypes::NumberOfElements);
            return static_cast<size_t>(_meshType);
        }

        // -------------------------------------------------------------------------------------------------------------------------

    }

    // -------------------------------------------------------------------------------------------------------------------------

    sMeshData& ShapeMeshLibrary::GetMeshData(sMeshTypes::Enum _meshType)
    {
        return GetShapeMeshes()[GetMeshIndex(_meshType)];
    }

    // -------------------------------------------------------------------------------------------------------------------------

    const sBounds& ShapeMeshLibrary::GetBounds(sMeshTypes::Enum _meshType)
    {
        return GetMeshData(_meshType).bounds;
    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------