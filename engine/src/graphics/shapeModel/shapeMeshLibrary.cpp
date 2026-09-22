#include "shapeMeshLibrary.h"

#include "graphics/shapeModel/meshGenerator.h"
#include "graphics/shapeModel/shapeModelDesc.h"
#include "math/matrix4x4.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <iterator>
#include <limits>

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
            meshes[static_cast<size_t>(sMeshTypes::ChunkPlane)] = cMeshGenerator::CreatePlane(sPlaneDesc{64, 64, 64, 64});
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
            meshes[static_cast<size_t>(sMeshTypes::Triangle)] = cMeshGenerator::CreateTriangle();

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

    std::vector<sShapeTriangleBatch> ShapeMeshLibrary::BakeTriangleModel(const sShapeModelDesc& _rModel)
    {
        using namespace Math;

        for (const sShapePartDesc& part : _rModel.shapes)
        {
            if (part.meshType != sMeshTypes::Triangle)
                return {};

            for (float value : { part.transform.position.x(), part.transform.position.y(), part.transform.position.z(),
                part.transform.rotation.x(), part.transform.rotation.y(), part.transform.rotation.z() })
            {
                if (!std::isfinite(value))
                    return {};
            }
            for (float scale : { part.transform.scale.x(), part.transform.scale.y(), part.transform.scale.z() })
            {
                if (!std::isfinite(scale) || scale <= 0.0f)
                    return {};
            }
        }

        std::vector<sShapeTriangleBatch> batches;
        const sMeshData& source = GetMeshData(sMeshTypes::Triangle);
        for (const sShapePartDesc& part : _rModel.shapes)
        {
            const std::array<float, 4> color = { part.color[0], part.color[1], part.color[2], part.color[3] };
            auto batch = std::find_if(batches.begin(), batches.end(), [&](const sShapeTriangleBatch& _rBatch)
            {
                return _rBatch.materialIndex == part.materialIndex && _rBatch.color == color;
            });
            if (batch == batches.end())
            {
                batches.emplace_back();
                batch = std::prev(batches.end());
                batch->color = color;
                batch->materialIndex = part.materialIndex;
                batch->mesh.pDebugName = "BakedShapeTriangles";
                const float limit = std::numeric_limits<float>::max();
                batch->mesh.bounds.min = { limit, limit, limit };
                batch->mesh.bounds.max = { -limit, -limit, -limit };
            }

            const auto& transform = part.transform;
            const cMatrix4x4f rotation = cMatrix4x4f::rotationX(transform.rotation.x())
                * cMatrix4x4f::rotationY(transform.rotation.y()) * cMatrix4x4f::rotationZ(transform.rotation.z());
            const cMatrix4x4f matrix = cMatrix4x4f::scale(transform.scale) * rotation
                * cMatrix4x4f::translation(transform.position);
            const uint32_t offset = static_cast<uint32_t>(batch->mesh.vertices.size());
            for (const sVertex& vertex : source.vertices)
            {
                sVertex baked = vertex;
                baked.position = matrix.transformPoint(vertex.position);
                // Triangle normals are +Z; positive diagonal scaling does not change their direction.
                baked.normal = rotation.transformDirection(vertex.normal).normalized();
                batch->mesh.vertices.push_back(baked);
                batch->mesh.bounds.min =
                {
                    std::min(batch->mesh.bounds.min.x(), baked.position.x()),
                    std::min(batch->mesh.bounds.min.y(), baked.position.y()),
                    std::min(batch->mesh.bounds.min.z(), baked.position.z())
                };
                batch->mesh.bounds.max =
                {
                    std::max(batch->mesh.bounds.max.x(), baked.position.x()),
                    std::max(batch->mesh.bounds.max.y(), baked.position.y()),
                    std::max(batch->mesh.bounds.max.z(), baked.position.z())
                };
            }
            for (uint32_t index : source.indices)
                batch->mesh.indices.push_back(offset + index);
        }

        for (sShapeTriangleBatch& batch : batches)
        {
            batch.mesh.bounds.center = (batch.mesh.bounds.min + batch.mesh.bounds.max) * 0.5f;
            batch.mesh.bounds.size = batch.mesh.bounds.max - batch.mesh.bounds.min;
            batch.mesh.bounds.radius = batch.mesh.bounds.size.length() * 0.5f;
        }
        return batches;
    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------
