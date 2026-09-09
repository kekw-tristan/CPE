#include "prefab.h"

#include "graphics/shapeModel/prefabDesc.h"
#include "graphics/shapeModel/prefabManager.h"

#include "graphics/scene/scene.h"

#include <cmath>

// -------------------------------------------------------------------------------------------------------------------------

namespace World
{

    // -------------------------------------------------------------------------------------------------------------------------

    using namespace Engine;

    // -------------------------------------------------------------------------------------------------------------------------

    namespace
    {

        // -------------------------------------------------------------------------------------------------------------------------

        Math::cVec3f Multiply(const Math::cVec3f& _rLeft, const Math::cVec3f& _rRight)
        {
            return Math::cVec3f(
                _rLeft.x() * _rRight.x(),
                _rLeft.y() * _rRight.y(),
                _rLeft.z() * _rRight.z()
            );
        }

        // -------------------------------------------------------------------------------------------------------------------------

        Math::cVec3f RotateX(const Math::cVec3f& _rVector, float _angle)
        {
            const float cosine = std::cos(_angle);
            const float sine = std::sin(_angle);

            return Math::cVec3f(
                _rVector.x(),
                _rVector.y() * cosine - _rVector.z() * sine,
                _rVector.y() * sine + _rVector.z() * cosine
            );
        }

        // -------------------------------------------------------------------------------------------------------------------------

        Math::cVec3f RotateY(const Math::cVec3f& _rVector, float _angle)
        {
            const float cosine = std::cos(_angle);
            const float sine = std::sin(_angle);

            return Math::cVec3f(
                _rVector.x() * cosine + _rVector.z() * sine,
                _rVector.y(),
                -_rVector.x() * sine + _rVector.z() * cosine
            );
        }

        // -------------------------------------------------------------------------------------------------------------------------

        Math::cVec3f RotateZ(const Math::cVec3f& _rVector, float _angle)
        {
            const float cosine = std::cos(_angle);
            const float sine = std::sin(_angle);

            return Math::cVec3f(
                _rVector.x() * cosine - _rVector.y() * sine,
                _rVector.x() * sine + _rVector.y() * cosine,
                _rVector.z()
            );
        }

        // -------------------------------------------------------------------------------------------------------------------------

        Math::cVec3f RotateEuler(const Math::cVec3f& _rVector, const Math::cVec3f& _rRotation)
        {
            Math::cVec3f result = _rVector;

            result = RotateX(result, _rRotation.x());
            result = RotateY(result, _rRotation.y());
            result = RotateZ(result, _rRotation.z());

            return result;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        GFX::sTransform CombineTransforms(const GFX::sTransform& _rParentTransform, const GFX::sTransform& _rLocalTransform)
        {
            GFX::sTransform transform{};


            const Math::cVec3f scaledLocalPosition = Multiply(
                _rLocalTransform.position,
                _rParentTransform.scale
            );

            const Math::cVec3f rotatedLocalPosition = RotateEuler(
                scaledLocalPosition,
                _rParentTransform.rotation
            );


            transform.position = _rParentTransform.position + rotatedLocalPosition;

            transform.rotation = _rParentTransform.rotation + _rLocalTransform.rotation;

            transform.scale = Multiply(
                _rParentTransform.scale,
                _rLocalTransform.scale
            );


            return transform;
        }

        // -------------------------------------------------------------------------------------------------------------------------

    }

    // -------------------------------------------------------------------------------------------------------------------------

    void InstantiatePrefab(
        GFX::cScene& _rScene,
        GFX::PrefabHandle _prefabHandle,
        const GFX::sTransform& _rTransform
    )
    {
        const GFX::sPrefabDesc& prefab = GFX::PrefabManager::GetPrefab(_prefabHandle);

        for (const GFX::sPrefabObjectDesc& object : prefab.objects)
        {
            if (object.modelHandle < 0)
                continue;

            GFX::sShapeInstance instance{};

            instance.modelHandle = object.modelHandle;
            instance.generateLights = object.generateLights;
            instance.collisionMode = object.generateColliders
                ? GFX::eShapeCollisionMode::Mesh
                : GFX::eShapeCollisionMode::Disabled;
            instance.transform = CombineTransforms(_rTransform, object.transform);

            _rScene.AddShapeInstance(instance);
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------
