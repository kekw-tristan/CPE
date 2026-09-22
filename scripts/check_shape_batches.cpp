// Standalone CPU check; link shapeMeshLibrary.cpp and meshGenerator.cpp.
#include "graphics/shapeModel/shapeMeshLibrary.h"
#include "graphics/shapeModel/shapeModelDesc.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace
{
    void Require(bool _condition, const char* _pMessage)
    {
        if (!_condition)
            throw std::runtime_error(_pMessage);
    }
}

int main()
{
    try
    {
        using namespace Engine::GFX;
        using namespace Engine::Math;

        sShapePartDesc part{};
        part.meshType = sMeshTypes::Triangle;
        part.transform.position = { -4.0f, 5.0f, 6.0f };
        part.transform.scale = { 2.0f, 3.0f, 1.0f };
        part.transform.rotation = { 0.0f, 1.57079632679f, 0.0f };
        for (float& color : part.color)
            color = 1.0f;
        part.materialIndex = 3;

        sShapeModelDesc model{};
        Require(ShapeMeshLibrary::BakeTriangleModel(model).empty(), "Empty model was baked");
        model.shapes.push_back(part);
        part.transform.position = { 3.0f, -2.0f, 1.0f };
        part.transform.rotation = { .35f, -.6f, 1.2f };
        model.shapes.push_back(part);
        part.materialIndex = 4;
        model.shapes.push_back(part);
        part.materialIndex = 3;
        part.color[0] = .25f;
        model.shapes.push_back(part);

        const auto batches = ShapeMeshLibrary::BakeTriangleModel(model);
        Require(batches.size() == 3, "Material/tint grouping failed");
        Require(batches[0].mesh.vertices.size() == 6, "Shared material did not combine");
        Require(batches[1].materialIndex == 4 && batches[2].color[0] == .25f, "Appearance changed");
        const auto& vertices = batches[0].mesh.vertices;
        Require((vertices[0].position - cVec3f(-4, 5, 6)).length() < 1e-5f, "Translation changed");
        Require((vertices[1].position - cVec3f(-4, 5, 4)).length() < 1e-5f, "Scale/rotation order changed");
        Require((vertices[2].position - cVec3f(-4, 8, 6)).length() < 1e-5f, "Triangle shape changed");
        Require((vertices[0].normal - cVec3f(1, 0, 0)).length() < 1e-5f, "Normal rotation changed");

        size_t triangleCount = 0;
        for (const auto& batch : batches)
        {
            const auto& mesh = batch.mesh;
            for (size_t i = 0; i < mesh.indices.size(); i += 3)
            {
                for (size_t j = 0; j < 3; ++j)
                    Require(mesh.indices[i + j] < mesh.vertices.size(), "Invalid rebased index");
                const auto& a = mesh.vertices[mesh.indices[i]];
                const auto& b = mesh.vertices[mesh.indices[i + 1]];
                const auto& c = mesh.vertices[mesh.indices[i + 2]];
                const cVec3f normal = (b.position - a.position).cross(c.position - a.position).normalized();
                Require(normal.dot(a.normal) > .99999f, "Normal/winding mismatch");
                ++triangleCount;
            }
            for (const auto& vertex : mesh.vertices)
            {
                Require(vertex.position.x() >= mesh.bounds.min.x() && vertex.position.x() <= mesh.bounds.max.x()
                    && vertex.position.y() >= mesh.bounds.min.y() && vertex.position.y() <= mesh.bounds.max.y()
                    && vertex.position.z() >= mesh.bounds.min.z() && vertex.position.z() <= mesh.bounds.max.z(),
                    "Bounds do not enclose mesh");
            }
        }
        Require(triangleCount == model.shapes.size(), "Triangle count changed");
        model.shapes[0].meshType = sMeshTypes::Cube;
        Require(ShapeMeshLibrary::BakeTriangleModel(model).empty(), "Mixed model lost non-triangle parts");
        model.shapes[0].meshType = sMeshTypes::Triangle;
        model.shapes[0].transform.scale = { -1, 1, 1 };
        Require(ShapeMeshLibrary::BakeTriangleModel(model).empty(), "Mirrored model accepted");
        model.shapes[0].transform.scale = { 1, 0, 1 };
        Require(ShapeMeshLibrary::BakeTriangleModel(model).empty(), "Degenerate model accepted");
        model.shapes[0].transform.scale = { 1, 1, 1 };
        model.shapes[0].transform.position = { std::numeric_limits<float>::infinity(), 0, 0 };
        Require(ShapeMeshLibrary::BakeTriangleModel(model).empty(), "Non-finite model accepted");
        std::cout << "Triangle batching: transforms, normals, winding, bounds, grouping and fallbacks passed.\n";
        return 0;
    }
    catch (const std::exception& _rError)
    {
        std::cerr << _rError.what() << '\n';
        return 1;
    }
}
