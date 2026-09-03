#include "collisionWorld.h"

#include "math/vector3.h"
#include "physics/collider.h"
#include "physics/collisionDetection.h"

#include <vector>

// -------------------------------------------------------------------------------------------------------------------------

namespace Engine::Physics
{

    // -------------------------------------------------------------------------------------------------------------------------

    namespace
    {
        class cCollisionWorld
        {

            public:

                static cCollisionWorld& GetInstance();

            public:

                void AddCollider(const sAABBCollider& _rCollider);
                void Clear();
                Math::cVec3f MoveCapsule(const sCapsuleCollider& _rCapsule, const Math::cVec3f& _rMovement);

            private:

                cCollisionWorld();
                ~cCollisionWorld(); 

                cCollisionWorld(const cCollisionWorld&)              = delete;
                cCollisionWorld& operator=(const cCollisionWorld&)   = delete; 

                cCollisionWorld(const cCollisionWorld&&)             = delete;
                cCollisionWorld& operator=(const cCollisionWorld&&)  = delete;

            private:

                std::vector<sAABBCollider> m_colliders;
        };
        
    }

    // -------------------------------------------------------------------------------------------------------------------------

    namespace
    {

        // -------------------------------------------------------------------------------------------------------------------------

        cCollisionWorld& cCollisionWorld::GetInstance()
        {
            static cCollisionWorld s_instance;
            return s_instance;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        void cCollisionWorld::AddCollider(const sAABBCollider& _rCollider)
        {
            m_colliders.push_back(_rCollider);
        }

        // -------------------------------------------------------------------------------------------------------------------------

        void cCollisionWorld::Clear()
        {
            m_colliders.clear();
        }

        // -------------------------------------------------------------------------------------------------------------------------

        Math::cVec3f cCollisionWorld::MoveCapsule(const sCapsuleCollider& _rCapsule, const Math::cVec3f& _rMovement)
        {
            sCapsuleCollider movedCapsule = _rCapsule;
            movedCapsule.center += _rMovement;

            constexpr uint32_t c_maxIterations = 4;

            for (uint32_t iteration = 0; iteration < c_maxIterations; ++iteration)
            {
                bool collisionFound = false;

                for (const sAABBCollider& collider : m_colliders)
                {
                    sCollisionResult result{};

                    if (!IntersectCapsuleAABB(movedCapsule, collider, result))
                        continue;

                    movedCapsule.center += result.normal * result.penetrationDepth;

                    collisionFound = true;
                }

                if (!collisionFound)
                    break;
            }

            return movedCapsule.center;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        cCollisionWorld::cCollisionWorld()
        {
        }

        // -------------------------------------------------------------------------------------------------------------------------

        cCollisionWorld::~cCollisionWorld()
        {
        }

        // -------------------------------------------------------------------------------------------------------------------------

    }

    // -------------------------------------------------------------------------------------------------------------------------

    namespace CollisionWorld
    {

        // -------------------------------------------------------------------------------------------------------------------------

        void AddCollider(const sAABBCollider& _rCollider)
        {
            cCollisionWorld::GetInstance().AddCollider(_rCollider);
        }

        // -------------------------------------------------------------------------------------------------------------------------

        void Clear()
        {
            cCollisionWorld::GetInstance().Clear();
        }

        // -------------------------------------------------------------------------------------------------------------------------

        Math::cVec3f MoveCapsule(const sCapsuleCollider& _rCapsule, const Math::cVec3f& _rMovement)
        {
            return cCollisionWorld::GetInstance().MoveCapsule(_rCapsule, _rMovement);
        }

        // -------------------------------------------------------------------------------------------------------------------------
    }

    // -------------------------------------------------------------------------------------------------------------------------
}


