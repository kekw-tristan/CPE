#include "shapeModelManager.h"


#include "graphics/shapeModel/shapeMeshLibrary.h"
#include "graphics/shapeModel/shapeModelDesc.h"

#include "math/vector3.h"

#include <algorithm>
#include <array>
#include <assert.h>
#include <cmath>
#include <limits>
#include <vector>

// -------------------------------------------------------------------------------------------------------------------------

namespace Engine::GFX
{

    // -------------------------------------------------------------------------------------------------------------------------

    namespace
    {
        class cShapeModelManager
        {

            public:

                static cShapeModelManager& GetInstance();

            public:

                ShapeModelHandle CreateShapeModel(const sShapeModelDesc& _rDesc); 
                const sShapeModelDesc& GetShapeModel(ShapeModelHandle _shapeModelHandle);
                void UpdateShapeModel(ShapeModelHandle _shapeModelHandle, const sShapeModelDesc& _rDesc);

            private:

                cShapeModelManager();
               ~cShapeModelManager();

               cShapeModelManager(const cShapeModelManager&)        = delete;
               cShapeModelManager& operator=(cShapeModelManager&)   = delete; 
               
            private:

                static sBounds CalculateBounds(const sShapeModelDesc& _rDesc);
                static Math::cVec3f TransformPoint(const Math::cVec3f& _rPoint, const sTransform& _rTransform);

            private:

                std::vector<sShapeModelDesc> m_shapeModels;
        };
    }

    // -------------------------------------------------------------------------------------------------------------------------

    namespace 
    {

        // -------------------------------------------------------------------------------------------------------------------------

        cShapeModelManager::cShapeModelManager()
            : m_shapeModels()
        {
        }

        // -------------------------------------------------------------------------------------------------------------------------

        cShapeModelManager::~cShapeModelManager()
        {

        }

        // -------------------------------------------------------------------------------------------------------------------------


        cShapeModelManager& cShapeModelManager::GetInstance()
        {
            static cShapeModelManager s_instance;
            return s_instance;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        ShapeModelHandle cShapeModelManager::CreateShapeModel(const sShapeModelDesc& _rDesc)
        {
            sShapeModelDesc model = _rDesc;

            model.bounds = CalculateBounds(model);

            const ShapeModelHandle handle = static_cast<ShapeModelHandle>(m_shapeModels.size());

            m_shapeModels.push_back(std::move(model));

            return handle;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        const sShapeModelDesc& cShapeModelManager::GetShapeModel(ShapeModelHandle _shapeModelHandle)
        {
            assert(_shapeModelHandle >= 0 && _shapeModelHandle < m_shapeModels.size());
            return m_shapeModels[_shapeModelHandle];
        }

        // -------------------------------------------------------------------------------------------------------------------------

        void cShapeModelManager::UpdateShapeModel(ShapeModelHandle _shapeModelHandle, const sShapeModelDesc& _rDesc)
        {
            assert(_shapeModelHandle >= 0 && _shapeModelHandle < static_cast<ShapeModelHandle>(m_shapeModels.size()));

            sShapeModelDesc model = _rDesc;

            model.bounds = CalculateBounds(model);

            m_shapeModels[_shapeModelHandle] = std::move(model);
        }

        // -------------------------------------------------------------------------------------------------------------------------

        sBounds cShapeModelManager::CalculateBounds(const sShapeModelDesc& _rDesc)
        {
            sBounds bounds{};

            if (_rDesc.shapes.empty())
                return bounds;

            constexpr float c_floatMax = std::numeric_limits<float>::max();
            constexpr float c_floatMin = std::numeric_limits<float>::lowest();

            Math::cVec3f boundsMin(c_floatMax, c_floatMax, c_floatMax);
            Math::cVec3f boundsMax(c_floatMin, c_floatMin, c_floatMin);

            std::vector<Math::cVec3f> transformedCorners;
            transformedCorners.reserve(_rDesc.shapes.size() * 8);

            for (const sShapePartDesc& part : _rDesc.shapes)
            {
                const sBounds& meshBounds = ShapeMeshLibrary::GetBounds(part.meshType);

                const std::array<Math::cVec3f, 8> corners =
                {
                    Math::cVec3f(meshBounds.min.x(), meshBounds.min.y(), meshBounds.min.z()),
                    Math::cVec3f(meshBounds.max.x(), meshBounds.min.y(), meshBounds.min.z()),
                    Math::cVec3f(meshBounds.min.x(), meshBounds.max.y(), meshBounds.min.z()),
                    Math::cVec3f(meshBounds.max.x(), meshBounds.max.y(), meshBounds.min.z()),
                    Math::cVec3f(meshBounds.min.x(), meshBounds.min.y(), meshBounds.max.z()),
                    Math::cVec3f(meshBounds.max.x(), meshBounds.min.y(), meshBounds.max.z()),
                    Math::cVec3f(meshBounds.min.x(), meshBounds.max.y(), meshBounds.max.z()),
                    Math::cVec3f(meshBounds.max.x(), meshBounds.max.y(), meshBounds.max.z())
                };

                for (const Math::cVec3f& corner : corners)
                {
                    const Math::cVec3f transformedCorner = TransformPoint(corner, part.transform);

                    boundsMin = Math::cVec3f(
                        std::min(boundsMin.x(), transformedCorner.x()),
                        std::min(boundsMin.y(), transformedCorner.y()),
                        std::min(boundsMin.z(), transformedCorner.z())
                    );

                    boundsMax = Math::cVec3f(
                        std::max(boundsMax.x(), transformedCorner.x()),
                        std::max(boundsMax.y(), transformedCorner.y()),
                        std::max(boundsMax.z(), transformedCorner.z())
                    );

                    transformedCorners.push_back(transformedCorner);
                }
            }

            bounds.min = boundsMin;
            bounds.max = boundsMax;
            bounds.center = (bounds.min + bounds.max) * 0.5f;
            bounds.size = bounds.max - bounds.min;

            float maximumRadiusSquared = 0.0f;

            for (const Math::cVec3f& corner : transformedCorners)
            {
                const Math::cVec3f difference = corner - bounds.center;
                maximumRadiusSquared = std::max(maximumRadiusSquared, difference.lengthSquared());
            }

            bounds.radius = std::sqrt(maximumRadiusSquared);

            return bounds;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        Math::cVec3f cShapeModelManager::TransformPoint(const Math::cVec3f& _rPoint, const sTransform& _rTransform)
        {
            Math::cVec3f point(
                _rPoint.x() * _rTransform.scale.x(),
                _rPoint.y() * _rTransform.scale.y(),
                _rPoint.z() * _rTransform.scale.z()
            );

            const float cosX = std::cos(_rTransform.rotation.x());
            const float sinX = std::sin(_rTransform.rotation.x());

            point = Math::cVec3f(
                point.x(),
                point.y() * cosX - point.z() * sinX,
                point.y() * sinX + point.z() * cosX
            );

            const float cosY = std::cos(_rTransform.rotation.y());
            const float sinY = std::sin(_rTransform.rotation.y());

            point = Math::cVec3f(
                point.x() * cosY + point.z() * sinY,
                point.y(),
                -point.x() * sinY + point.z() * cosY
            );

            const float cosZ = std::cos(_rTransform.rotation.z());
            const float sinZ = std::sin(_rTransform.rotation.z());

            point = Math::cVec3f(
                point.x() * cosZ - point.y() * sinZ,
                point.x() * sinZ + point.y() * cosZ,
                point.z()
            );

            point += _rTransform.position;

            return point;
        }

        // -------------------------------------------------------------------------------------------------------------------------

    }

    // -------------------------------------------------------------------------------------------------------------------------

    namespace ShapeModelManager
    {

        // -------------------------------------------------------------------------------------------------------------------------

        ShapeModelHandle CreateShapeModel(const sShapeModelDesc& _rDesc)
        {
            return cShapeModelManager::GetInstance().CreateShapeModel(_rDesc); 
        }

        // -------------------------------------------------------------------------------------------------------------------------

        const sShapeModelDesc& GetShapeModel(ShapeModelHandle _shapeModelHandle)
        {
            return cShapeModelManager::GetInstance().GetShapeModel(_shapeModelHandle);
        }

        // -------------------------------------------------------------------------------------------------------------------------

        void UpdateShapeModel(ShapeModelHandle _shapeModelHandle, const sShapeModelDesc& _rDesc)
        {
            cShapeModelManager::GetInstance().UpdateShapeModel(_shapeModelHandle, _rDesc);
        }

        // -------------------------------------------------------------------------------------------------------------------------

    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------