// Standalone CPU regression check; link collisionDetection.cpp and collisionWorld.cpp.
#include "physics/collisionDetection.h"
#include "physics/collisionWorld.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace
{
    void RequireNear(float _actual, float _expected, const char* _pMessage)
    {
        if (std::abs(_actual - _expected) > 1e-4f)
            throw std::runtime_error(_pMessage);
    }
}

int main()
{
    try
    {
        using namespace Engine::Math;
        using namespace Engine::Physics;

        const sAABBCollider wall{ .center = { 5, 0, 0 }, .halfExtents = { .01f, 4, 4 } };
        RequireNear(SweepSphereAABB({ 0, 0, 0 }, { 10, 0, 0 }, .3f, wall), .469f, "Thin wall was skipped");
        RequireNear(SweepSphereAABB({ 5, 0, 0 }, {}, .3f, wall), 0, "Initial overlap was ignored");
        RequireNear(SweepSphereAABB({ 0, 0, 0 }, {}, .3f, wall), 1, "Stationary clear sphere hit");
        RequireNear(SweepSphereAABB({ 0, 5, 0 }, { 10, 0, 0 }, .3f, wall), 1, "Clear path above wall hit");

        sTriangleCollider triangle{ { 5, -2, -2 }, { 5, 2, -2 }, { 5, 0, 2 } };
        RequireNear(SweepSphereTriangle({}, { 10, 0, 0 }, .3f, triangle), .47f, "Front triangle face missed");
        RequireNear(SweepSphereTriangle({ 10, 0, 0 }, { -10, 0, 0 }, .3f, triangle), .47f, "Back triangle face missed");
        std::swap(triangle.b, triangle.c);
        RequireNear(SweepSphereTriangle({}, { 10, 0, 0 }, .3f, triangle), .47f, "Triangle winding affects camera");
        RequireNear(SweepSphereTriangle({ 5, 0, 0 }, { 10, 0, 0 }, .3f, triangle), 0, "Triangle overlap ignored");
        RequireNear(SweepSphereTriangle({}, {}, .3f, triangle), 1, "Stationary sphere hit a distant triangle");
        RequireNear(SweepSphereTriangle({ 0, 0, 3 }, { 10, 0, 0 }, .3f, triangle), 1, "Triangle bounds caused false hit");

        const sTriangleCollider edge{ { -2, 0, 5 }, { 2, 0, 5 }, { 0, 3, 5 } };
        RequireNear(SweepSphereTriangle({ 0, -.2f, 0 }, { 0, 0, 10 }, .3f, edge),
            (5 - std::sqrt(.05f)) / 10, "Grazing edge contact missed");
        RequireNear(SweepSphereTriangle({ -2.2f, -.1f, 0 }, { 0, 0, 10 }, .3f, edge), .48f, "Vertex contact missed");
        RequireNear(SweepSphereTriangle({ -4, -.2f, 5 }, { 8, 0, 0 }, .3f, edge),
            (2 - std::sqrt(.05f)) / 8, "Parallel edge approach missed");

        const sTriangleCollider ceiling{ { -5, 2, -5 }, { 5, 2, -5 }, { 0, 2, 5 } };
        RequireNear(SweepSphereTriangle({ 0, 1, 0 }, { 0, 4, 0 }, .3f, ceiling), .175f, "Ceiling missed");
        const cVec3f shift{ 10000, -200, -10000 };
        const sTriangleCollider shifted{ triangle.a + shift, triangle.b + shift, triangle.c + shift };
        RequireNear(SweepSphereTriangle(shift, { 10, 0, 0 }, .3f, shifted), .47f, "Translated triangle missed");

        CollisionWorld::Clear();
        RequireNear(CollisionWorld::SweepSphere({}, { 10, 0, 0 }, .3f), 1, "Empty world hit");
        const auto wallHandle = CollisionWorld::AddCollider(wall);
        RequireNear(CollisionWorld::SweepSphere({}, { 10, 0, 0 }, .3f), .469f, "World wall query failed");
        CollisionWorld::RemoveCollider(wallHandle);
        const auto triangleHandle = CollisionWorld::AddCollider(triangle);
        CollisionWorld::RemoveCollider(wallHandle);
        RequireNear(CollisionWorld::SweepSphere({}, { 10, 0, 0 }, .3f), .47f, "Recycled collider slot lost triangle");
        CollisionWorld::RemoveCollider(triangleHandle);
        RequireNear(CollisionWorld::SweepSphere({}, { 10, 0, 0 }, .3f), 1, "Unloaded collider still obstructs camera");

        CollisionWorld::AddCollider(sAABBCollider{ .center = { -10, 0, -10 }, .halfExtents = { .1f, 3, 3 } });
        RequireNear(CollisionWorld::SweepSphere({ -15, 0, -10 }, { 10, 0, 0 }, .3f), .46f, "Negative spatial cells missed");
        CollisionWorld::Clear();
        CollisionWorld::AddCollider(sAABBCollider{ .center = { 0, 0, 0 }, .halfExtents = { 20, 2, 20 },
            .isGround = true, .groundHeightSampler = [](float, float) { return -1.0f; } });
        RequireNear(CollisionWorld::SweepSphere({}, { 10, 0, 0 }, .3f), 1, "Terrain proxy box blocked camera");
        CollisionWorld::AddCollider(ceiling);
        RequireNear(CollisionWorld::SweepSphere({ 0, 1, 0 }, { 0, 4, 0 }, .3f), .175f, "World ceiling query failed");
        CollisionWorld::Clear();

        // A shoulder offset must be clipped before the backwards camera arm.
        CollisionWorld::AddCollider(sAABBCollider{ .center = { .6f, 2, 0 }, .halfExtents = { .05f, 2, 5 } });
        const cVec3f head{ 0, 1.8f, 0 };
        const cVec3f shoulder{ .75f, 0, 0 };
        const float fraction = CollisionWorld::SweepSphere(head, shoulder, .3f);
        RequireNear(fraction, 1.0f / 3.0f, "Shoulder crossed side wall");
        const cVec3f safeTarget = head + shoulder * std::max(0.0f, fraction - .03f / shoulder.length());
        RequireNear(CollisionWorld::SweepSphere(safeTarget, { 0, 0, -6 }, .3f), 1, "Safe shoulder blocked camera arm");
        CollisionWorld::Clear();
        std::cout << "Camera sweep: thin walls, two-sided triangles, edges, vertices, ceilings, shoulder and streaming passed.\n";
        return 0;
    }
    catch (const std::exception& _rError)
    {
        std::cerr << _rError.what() << '\n';
        return 1;
    }
}
