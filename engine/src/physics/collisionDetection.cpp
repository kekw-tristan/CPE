#include "collisionDetection.h"

#include <algorithm>
#include <cmath>

// -------------------------------------------------------------------------------------------------------------------------

namespace Engine::Physics
{

    namespace
    {
        using Math::cVec3f;

        cVec3f ClosestPointOnTriangle(const cVec3f& _rPoint, const sTriangleCollider& _rTriangle)
        {
            const cVec3f ab = _rTriangle.b - _rTriangle.a;
            const cVec3f ac = _rTriangle.c - _rTriangle.a;
            const cVec3f ap = _rPoint - _rTriangle.a;
            const float d1 = ab.dot(ap);
            const float d2 = ac.dot(ap);
            if (d1 <= 0.0f && d2 <= 0.0f)
            {
                return _rTriangle.a;
            }

            const cVec3f bp = _rPoint - _rTriangle.b;
            const float d3 = ab.dot(bp);
            const float d4 = ac.dot(bp);
            if (d3 >= 0.0f && d4 <= d3)
            {
                return _rTriangle.b;
            }

            const float vc = d1 * d4 - d3 * d2;
            if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f)
            {
                return _rTriangle.a + ab * (d1 / (d1 - d3));
            }

            const cVec3f cp = _rPoint - _rTriangle.c;
            const float d5 = ab.dot(cp);
            const float d6 = ac.dot(cp);
            if (d6 >= 0.0f && d5 <= d6)
            {
                return _rTriangle.c;
            }

            const float vb = d5 * d2 - d1 * d6;
            if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f)
            {
                return _rTriangle.a + ac * (d2 / (d2 - d6));
            }

            const float va = d3 * d6 - d5 * d4;
            if (va <= 0.0f && d4 >= d3 && d5 >= d6)
            {
                return _rTriangle.b + (_rTriangle.c - _rTriangle.b) * ((d4 - d3) / ((d4 - d3) + (d5 - d6)));
            }

            const float inverse = 1.0f / (va + vb + vc);
            return _rTriangle.a + ab * (vb * inverse) + ac * (vc * inverse);
        }

        // -------------------------------------------------------------------------------------------------------------------------

        void ClosestSegmentPoints(const cVec3f& _rA, const cVec3f& _rB, const cVec3f& _rC, const cVec3f& _rD,
            cVec3f& _rFirst, cVec3f& _rSecond)
        {
            const cVec3f u = _rB - _rA;
            const cVec3f v = _rD - _rC;
            const cVec3f offset = _rA - _rC;
            const float a = u.dot(u);
            const float b = u.dot(v);
            const float c = v.dot(v);
            const float d = u.dot(offset);
            const float e = v.dot(offset);
            const float denominator = a * c - b * b;
            float s = denominator > 0.0f ? std::clamp((b * e - c * d) / denominator, 0.0f, 1.0f) : 0.0f;
            float t = c > 0.0f ? (b * s + e) / c : 0.0f;
            if (t < 0.0f)
            {
                t = 0.0f;
                s = a > 0.0f ? std::clamp(-d / a, 0.0f, 1.0f) : 0.0f;
            }
            else if (t > 1.0f)
            {
                t = 1.0f;
                s = a > 0.0f ? std::clamp((b - d) / a, 0.0f, 1.0f) : 0.0f;
            }
            _rFirst = _rA + u * s;
            _rSecond = _rC + v * t;
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

    bool IntersectCapsuleTriangle(const sCapsuleCollider& _rCapsule, const sTriangleCollider& _rTriangle, sCollisionResult& _rResult)
    {
        _rResult = {};
        const Math::cVec3f normal = (_rTriangle.b - _rTriangle.a).cross(_rTriangle.c - _rTriangle.a).normalized();
        if (normal.isZero())
        {
            return false;
        }

        const Math::cVec3f start = _rCapsule.center - Math::cVec3f{ 0.0f, _rCapsule.halfHeight, 0.0f };
        const Math::cVec3f end = _rCapsule.center + Math::cVec3f{ 0.0f, _rCapsule.halfHeight, 0.0f };
        Math::cVec3f closestAxis = start;
        Math::cVec3f closestSurface = ClosestPointOnTriangle(start, _rTriangle);
        float distanceSquared = (closestAxis - closestSurface).lengthSquared();
        const auto consider = [&](const Math::cVec3f& _rAxis, const Math::cVec3f& _rSurface)
        {
            const float candidate = (_rAxis - _rSurface).lengthSquared();
            if (candidate < distanceSquared)
            {
                distanceSquared = candidate;
                closestAxis = _rAxis;
                closestSurface = _rSurface;
            }
        };
        consider(end, ClosestPointOnTriangle(end, _rTriangle));

        const float startDistance = normal.dot(start - _rTriangle.a);
        const float endDistance = normal.dot(end - _rTriangle.a);
        if (startDistance != endDistance)
        {
            const float factor = startDistance / (startDistance - endDistance);
            if (factor >= 0.0f && factor <= 1.0f)
            {
                const Math::cVec3f intersection = start + (end - start) * factor;
                consider(intersection, ClosestPointOnTriangle(intersection, _rTriangle));
            }
        }

        const Math::cVec3f vertices[] = { _rTriangle.a, _rTriangle.b, _rTriangle.c };
        for (int edge = 0; edge < 3; ++edge)
        {
            Math::cVec3f axis;
            Math::cVec3f surface;
            ClosestSegmentPoints(start, end, vertices[edge], vertices[(edge + 1) % 3], axis, surface);
            consider(axis, surface);
        }

        if (distanceSquared >= _rCapsule.radius * _rCapsule.radius)
        {
            return false;
        }

        _rResult.collided = true;
        const float distance = std::sqrt(distanceSquared);
        if (distance > 0.00001f)
        {
            _rResult.normal = (closestAxis - closestSurface) / distance;
            _rResult.penetrationDepth = _rCapsule.radius - distance;
        }
        else
        {
            const bool front = startDistance + endDistance >= 0.0f;
            _rResult.normal = front ? normal : -normal;
            _rResult.penetrationDepth = _rCapsule.radius
                + (front ? -std::min(startDistance, endDistance) : std::max(startDistance, endDistance));
        }
        return true;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    bool FindTriangleGroundHeight(const Math::cVec3f& _rPosition, const sTriangleCollider& _rTriangle, float& _rHeight)
    {
        const Math::cVec3f normal = (_rTriangle.b - _rTriangle.a).cross(_rTriangle.c - _rTriangle.a).normalized();
        // Surfaces steeper than 60 degrees remain obstacles rather than walkable ground.
        if (normal.y() < 0.5f)
        {
            return false;
        }

        const Math::cVec3f ab = _rTriangle.b - _rTriangle.a;
        const Math::cVec3f ac = _rTriangle.c - _rTriangle.a;
        const Math::cVec3f offset = _rPosition - _rTriangle.a;
        const double denominator = static_cast<double>(ab.x()) * ac.z() - static_cast<double>(ab.z()) * ac.x();
        const double u = (static_cast<double>(offset.x()) * ac.z() - static_cast<double>(offset.z()) * ac.x()) / denominator;
        const double v = (static_cast<double>(ab.x()) * offset.z() - static_cast<double>(ab.z()) * offset.x()) / denominator;
        if (u < -0.000001 || v < -0.000001 || u + v > 1.000001)
        {
            return false;
        }

        _rHeight = _rTriangle.a.y() + static_cast<float>(u) * ab.y() + static_cast<float>(v) * ac.y();
        return true;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    bool IntersectCapsuleAABB(const sCapsuleCollider& _rCapsule, const sAABBCollider& _rBox, sCollisionResult& _rResult)
    {
        const Math::cVec3f boxMin = _rBox.center - _rBox.halfExtents;
        const Math::cVec3f boxMax = _rBox.center + _rBox.halfExtents;

        const float capsuleMinY = _rCapsule.center.y() - _rCapsule.halfHeight;
        const float capsuleMaxY = _rCapsule.center.y() + _rCapsule.halfHeight;

        const float closestBoxX = std::clamp(_rCapsule.center.x(), boxMin.x(), boxMax.x());
        const float closestBoxZ = std::clamp(_rCapsule.center.z(), boxMin.z(), boxMax.z());

        float closestCapsuleY;
        float closestBoxY;

        if (capsuleMaxY < boxMin.y())
        {
            closestCapsuleY = capsuleMaxY;
            closestBoxY     = boxMin.y();
        }
        else if (capsuleMinY > boxMax.y())
        {
            closestCapsuleY = capsuleMinY;
            closestBoxY     = boxMax.y();
        }
        else
        {
            closestCapsuleY = std::clamp(_rCapsule.center.y(), boxMin.y(), boxMax.y());
            closestBoxY     = closestCapsuleY;
        }

        Math::cVec3f difference(
            _rCapsule.center.x() - closestBoxX,
            closestCapsuleY      - closestBoxY,
            _rCapsule.center.z() - closestBoxZ);

        const float distanceSquared =
            difference.x() * difference.x() +
            difference.y() * difference.y() +
            difference.z() * difference.z();

        const float radiusSquared = _rCapsule.radius * _rCapsule.radius;

        if (distanceSquared >= radiusSquared)
        {
            _rResult = {};
            return false;
        }

        _rResult.collided = true;

        const float distance = std::sqrt(distanceSquared);

        if (distance > 0.0001f)
        {
            _rResult.normal           = difference * (1.0f / distance);
            _rResult.penetrationDepth = _rCapsule.radius - distance;

            return true;
        }

        // Capsule axis is already inside the AABB.
        // Choose the nearest horizontal box face.
        const float distanceLeft    = _rCapsule.center.x() - boxMin.x();
        const float distanceRight   = boxMax.x() - _rCapsule.center.x();
        const float distanceBack    = _rCapsule.center.z() - boxMin.z();
        const float distanceFront   = boxMax.z() - _rCapsule.center.z();

        float minDistance = distanceLeft;

        _rResult.normal = Math::cVec3f(-1.0f, 0.0f, 0.0f);

        if (distanceRight < minDistance)
        {
            minDistance = distanceRight;
            _rResult.normal = Math::cVec3f(1.0f, 0.0f, 0.0f);
        }

        if (distanceBack < minDistance)
        {
            minDistance = distanceBack;
            _rResult.normal = Math::cVec3f(0.0f, 0.0f, -1.0f);
        }

        if (distanceFront < minDistance)
        {
            minDistance = distanceFront;
            _rResult.normal = Math::cVec3f(0.0f, 0.0f, 1.0f);
        }

        _rResult.penetrationDepth = _rCapsule.radius + minDistance;

        return true;
    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------
