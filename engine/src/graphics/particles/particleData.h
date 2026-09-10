#pragma once

#include "math/vector3.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace Engine::GFX
{
    inline constexpr size_t c_maxParticles = 8192;
    inline constexpr size_t c_maxParticleSurfaces = 32768;
    inline constexpr size_t c_maxParticleDraws = c_maxParticles + c_maxParticleSurfaces;

    // Five float4 attributes shared with particles.hlsl. Normal.w selects the visual shape.
    struct sParticleData
    {
        std::array<float, 4> positionSize{};
        std::array<float, 4> color{};
        std::array<float, 4> normalMode{};
        std::array<float, 4> rotationAge{};
        std::array<float, 4> surfaceClip{};
    };

    static_assert(sizeof(sParticleData) == 80);

    struct sParticleEmitterHandle
    {
        uint32_t index = std::numeric_limits<uint32_t>::max();
        uint32_t generation = 0;

        bool operator==(const sParticleEmitterHandle&) const = default;
    };

    struct sParticleSurface
    {
        Math::cVec3f position{};
        Math::cVec3f normal = { 0.0f, 1.0f, 0.0f };
        float radius = 0.0f;
        float tileHalfExtent = 0.0f;
        // X/Z center and radius of the entire area; tiles share one continuous mask.
        std::array<float, 3> areaClip{};
    };

    enum class eParticleAppearance
    {
        Soft,
        Vapor,
        Bubble
    };

    struct sParticleDefinition
    {
        eParticleAppearance appearance = eParticleAppearance::Soft;
        float spawnRate = 20.0f;
        float lifetime = 1.0f;
        float startSize = 0.1f;
        float endSize = 0.3f;
        float speed = 0.5f;
        float spread = 0.2f;
        Math::cVec3f acceleration{};
        std::array<float, 4> startColor = { 1.0f, 1.0f, 1.0f, 0.7f };
        std::array<float, 4> endColor = { 1.0f, 1.0f, 1.0f, 0.0f };
    };
}
