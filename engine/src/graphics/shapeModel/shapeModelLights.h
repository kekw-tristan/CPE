#pragma once

#include "graphics/light/lightManager.h"
#include "graphics/shapeModel/shapeModelDesc.h"

#include "math/matrix4x4.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace Engine::GFX::ShapeModelLights
{
    inline sLight CreateWorldLight(const sShapeLightDesc& _rLightDesc, const sTransform& _rTransform)
    {
        using Math::cMatrix4x4f;

        const cMatrix4x4f rotation = cMatrix4x4f::rotationX(_rTransform.rotation.x())
            * cMatrix4x4f::rotationY(_rTransform.rotation.y())
            * cMatrix4x4f::rotationZ(_rTransform.rotation.z());
        const cMatrix4x4f world = cMatrix4x4f::scale(_rTransform.scale)
            * rotation
            * cMatrix4x4f::translation(_rTransform.position);

        const float maximumScale = std::max(
            std::abs(_rTransform.scale.x()),
            std::max(std::abs(_rTransform.scale.y()), std::abs(_rTransform.scale.z()))
        );

        Math::cVec3f direction = rotation.transformDirection(_rLightDesc.direction);

        if (direction.isZero())
            direction = { 0.0f, 0.0f, -1.0f };
        else
            direction.normalize();

        sLight light{};

        light.type        = _rLightDesc.type;
        light.color       = _rLightDesc.color;
        light.intensity   = maximumScale > 0.0001f ? _rLightDesc.intensity : 0.0f;
        light.position    = world.transformPoint(_rLightDesc.position);
        light.radius      = std::max(_rLightDesc.radius * maximumScale, 0.0001f);
        light.direction   = direction;
        light.innerCone   = std::cos(_rLightDesc.innerConeAngle);
        light.outerCone   = std::cos(_rLightDesc.outerConeAngle);
        light.castsShadow = _rLightDesc.castsShadow;

        return light;
    }

    inline sLight CreateWorldLight(
        const sShapeLightDesc& _rFrom,
        const sShapeLightDesc& _rTo,
        float _weight,
        const sTransform& _rTransform)
    {
        const float weight = std::clamp(_weight, 0.0f, 1.0f);
        const sLight from  = CreateWorldLight(_rFrom, _rTransform);
        const sLight to    = CreateWorldLight(_rTo, _rTransform);

        sLight light{};

        light.type        = from.type;
        light.color       = from.color + (to.color - from.color) * weight;
        light.intensity   = from.intensity + (to.intensity - from.intensity) * weight;
        light.position    = from.position + (to.position - from.position) * weight;
        light.radius      = from.radius + (to.radius - from.radius) * weight;
        light.direction   = from.direction + (to.direction - from.direction) * weight;
        light.innerCone   = from.innerCone + (to.innerCone - from.innerCone) * weight;
        light.outerCone   = from.outerCone + (to.outerCone - from.outerCone) * weight;
        light.castsShadow = from.castsShadow;

        if (light.direction.isZero())
            light.direction = from.direction;
        else
            light.direction.normalize();

        return light;
    }

    inline void Create(
        const sShapeModelDesc& _rModel,
        const sTransform& _rTransform,
        std::vector<LightHandle>& _rLightHandles)
    {
        for (LightHandle lightHandle : _rLightHandles)
            LightManager::DestroyLight(lightHandle);

        _rLightHandles.clear();
        _rLightHandles.reserve(_rModel.lights.size());

        for (const sShapeLightDesc& lightDesc : _rModel.lights)
            _rLightHandles.push_back(LightManager::CreateLight(CreateWorldLight(lightDesc, _rTransform)));
    }

    inline void Update(
        const sShapeModelDesc& _rModel,
        const sTransform& _rTransform,
        std::vector<LightHandle>& _rLightHandles)
    {
        if (_rLightHandles.size() != _rModel.lights.size())
        {
            Create(_rModel, _rTransform, _rLightHandles);
            return;
        }

        for (size_t lightIndex = 0; lightIndex < _rModel.lights.size(); ++lightIndex)
        {
            const sLight light = CreateWorldLight(_rModel.lights[lightIndex], _rTransform);

            if (!LightManager::UpdateLight(_rLightHandles[lightIndex], light))
                _rLightHandles[lightIndex] = LightManager::CreateLight(light);
        }
    }

    inline void Update(
        const sShapeModelDesc& _rModel,
        const sShapeModelDesc& _rPoseModel,
        float _weight,
        const sTransform& _rTransform,
        std::vector<LightHandle>& _rLightHandles)
    {
        if (_rPoseModel.lights.size() != _rModel.lights.size())
        {
            Update(_rModel, _rTransform, _rLightHandles);
            return;
        }

        if (_rLightHandles.size() != _rModel.lights.size())
            Create(_rModel, _rTransform, _rLightHandles);

        for (size_t lightIndex = 0; lightIndex < _rModel.lights.size(); ++lightIndex)
        {
            const sLight light = CreateWorldLight(
                _rModel.lights[lightIndex],
                _rPoseModel.lights[lightIndex],
                _weight,
                _rTransform
            );

            if (!LightManager::UpdateLight(_rLightHandles[lightIndex], light))
                _rLightHandles[lightIndex] = LightManager::CreateLight(light);
        }
    }

    inline void Destroy(std::vector<LightHandle>& _rLightHandles)
    {
        for (LightHandle lightHandle : _rLightHandles)
            LightManager::DestroyLight(lightHandle);

        _rLightHandles.clear();
    }
}
