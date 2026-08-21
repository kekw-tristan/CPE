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