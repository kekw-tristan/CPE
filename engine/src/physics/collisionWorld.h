#pragma once

#include <cstddef>
#include <cstdint>

namespace Engine::Math
{
    template<typename T>
    class cVector3;

    using cVec3f = cVector3<float>;
}

namespace Engine::Physics
{
    struct sColliderHandle
    {
        std::size_t index    = 0;
        uint64_t generation = 0;
    };

    struct sCapsuleCollider;
    struct sAABBCollider;
    struct sTriangleCollider;

    namespace CollisionWorld
    {
        sColliderHandle AddCollider(const sAABBCollider& _rCollider);
        sColliderHandle AddCollider(const sTriangleCollider& _rCollider);
        void RemoveCollider(sColliderHandle _handle);
        void Clear();

        Math::cVec3f MoveCapsule(const sCapsuleCollider& _rCapsule, const Math::cVec3f& _rMovement);
        Math::cVec3f MoveCapsule(const sCapsuleCollider& _rCapsule, const Math::cVec3f& _rMovement, float _maximumStepHeight);
        bool FindGroundHeight(const Math::cVec3f& _rPosition, float _maximumHeight, float& _rGroundHeight);
    }
}
