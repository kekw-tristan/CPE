#include "particleSystem.h"

#include <algorithm>
#include <cmath>

namespace Engine::GFX
{
    cParticleSystem::cParticleSystem()
        : m_emitters(c_maxEmitters)
    {
        m_particles.reserve(c_maxParticles);
        m_surfaces.reserve(c_maxParticleSurfaces);
        m_renderData.reserve(c_maxParticleDraws);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    bool cParticleSystem::IsAlive(sParticleEmitterHandle _handle) const
    {
        return _handle.index < m_emitters.size() && m_emitters[_handle.index].active
            && m_emitters[_handle.index].generation == _handle.generation;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    sParticleEmitterHandle cParticleSystem::CreateEmitter(const sParticleDefinition& _rDefinition, const Math::cVec3f& _rPosition)
    {
        for (uint32_t index = 0; index < m_emitters.size(); ++index)
        {
            sEmitter& emitter = m_emitters[index];
            if (emitter.active)
                continue;

            emitter.definition = _rDefinition;
            emitter.position = _rPosition;
            emitter.previousPosition = _rPosition;
            emitter.surfaceCount = 0;
            emitter.spawnRemainder = 0.0f;
            emitter.active = true;
            return { index, emitter.generation };
        }
        return {};
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cParticleSystem::SetPosition(sParticleEmitterHandle _handle, const Math::cVec3f& _rPosition)
    {
        if (IsAlive(_handle))
            m_emitters[_handle.index].position = _rPosition;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cParticleSystem::SetSurfaces(sParticleEmitterHandle _handle, std::span<const sParticleSurface> _surfaces)
    {
        if (!IsAlive(_handle))
            return;

        sEmitter& emitter = m_emitters[_handle.index];
        emitter.surfaceCount = std::min(_surfaces.size(), c_maxEmitterSurfaces);
        std::copy_n(_surfaces.begin(), emitter.surfaceCount, emitter.surfaces.begin());
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cParticleSystem::StopEmitter(sParticleEmitterHandle _handle, bool _clearParticles)
    {
        if (!IsAlive(_handle))
            return;

        sEmitter& emitter = m_emitters[_handle.index];
        emitter.active = false;
        ++emitter.generation;
        if (_clearParticles)
            std::erase_if(m_particles, [_handle](const sParticle& _rParticle) { return _rParticle.owner == _handle; });
    }

    // -------------------------------------------------------------------------------------------------------------------------

    float cParticleSystem::Random()
    {
        m_randomState ^= m_randomState << 13;
        m_randomState ^= m_randomState >> 17;
        m_randomState ^= m_randomState << 5;
        return static_cast<float>(m_randomState & 0x00ffffff) / 16777216.0f;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cParticleSystem::Emit(const sParticleDefinition& _rDefinition, const Math::cVec3f& _rPosition,
        const Math::cVec3f& _rNormal, sParticleEmitterHandle _owner, float _age)
    {
        if (_rDefinition.lifetime <= _age || _rDefinition.lifetime <= 0.0f)
            return;
        if (m_particles.size() == c_maxParticles)
        {
            ++m_droppedParticles;
            return;
        }

        sParticle particle{};
        particle.definition = _rDefinition;
        particle.owner = _owner;
        particle.velocity = _rNormal * _rDefinition.speed
            + Math::cVec3f(Random() - 0.5f, Random() * 0.5f, Random() - 0.5f) * _rDefinition.spread;
        particle.position = _rPosition + particle.velocity * _age + _rDefinition.acceleration * (0.5f * _age * _age);
        particle.velocity = particle.velocity + _rDefinition.acceleration * _age;
        particle.age = _age;
        particle.rotation = Random() * 6.2831853f;
        m_particles.push_back(particle);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cParticleSystem::Burst(const sParticleDefinition& _rDefinition, const Math::cVec3f& _rPosition, uint32_t _count)
    {
        const uint32_t count = std::min(_count, static_cast<uint32_t>(c_maxParticles));
        m_droppedParticles += _count - count;
        for (uint32_t index = 0; index < count; ++index)
            Emit(_rDefinition, _rPosition, { 0.0f, 1.0f, 0.0f }, {}, 0.0f);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cParticleSystem::Update(float _deltaTime)
    {
        if (!std::isfinite(_deltaTime) || _deltaTime <= 0.0f)
            return;

        for (sParticle& particle : m_particles)
        {
            particle.age += _deltaTime;
            particle.position = particle.position + particle.velocity * _deltaTime
                + particle.definition.acceleration * (0.5f * _deltaTime * _deltaTime);
            particle.velocity = particle.velocity + particle.definition.acceleration * _deltaTime;
        }
        std::erase_if(m_particles, [](const sParticle& _rParticle) { return _rParticle.age >= _rParticle.definition.lifetime; });

        for (uint32_t index = 0; index < m_emitters.size(); ++index)
        {
            sEmitter& emitter = m_emitters[index];
            if (!emitter.active)
                continue;

            const float rate = std::clamp(emitter.definition.spawnRate, 0.0f, 4096.0f);
            const float total = emitter.spawnRemainder + rate * _deltaTime;
            const float requested = std::floor(total);
            emitter.spawnRemainder = total - requested;
            const uint32_t count = static_cast<uint32_t>(std::min(requested, static_cast<float>(c_maxParticles)));
            m_droppedParticles += static_cast<uint64_t>(requested - count);
            for (uint32_t spawn = 0; spawn < count; ++spawn)
            {
                // Reconstruct births inside the frame, so trails and lifetimes survive frame stalls.
                const float age = (emitter.spawnRemainder + spawn) / std::max(rate, 0.001f);
                const float t = std::clamp(1.0f - age / _deltaTime, 0.0f, 1.0f);
                Math::cVec3f position = emitter.previousPosition * (1.0f - t) + emitter.position * t;
                Math::cVec3f normal(0.0f, 1.0f, 0.0f);
                if (emitter.surfaceCount > 0)
                {
                    const size_t surfaceIndex = std::min(static_cast<size_t>(Random() * emitter.surfaceCount), emitter.surfaceCount - 1);
                    const sParticleSurface& surface = emitter.surfaces[surfaceIndex];
                    if (surface.radius <= 0.0f)
                        continue;

                    const float angle = Random() * 6.2831853f;
                    const float radius = std::sqrt(Random()) * surface.radius * 0.65f;
                    const float x = std::cos(angle) * radius;
                    const float z = std::sin(angle) * radius;
                    normal = surface.normal;
                    position = surface.position + Math::cVec3f(x,
                        -(normal.x() * x + normal.z() * z) / std::max(normal.y(), 0.1f) + 0.08f, z);
                }
                Emit(emitter.definition, position, normal, { index, emitter.generation }, age);
            }
            emitter.previousPosition = emitter.position;
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cParticleSystem::Clear()
    {
        for (sEmitter& emitter : m_emitters)
        {
            emitter.active = false;
            ++emitter.generation;
        }
        m_particles.clear();
        m_surfaces.clear();
        m_renderData.clear();
        m_droppedParticles = 0;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cParticleSystem::RemoveInBounds(const Math::cVec3f& _rMinimum, const Math::cVec3f& _rMaximum)
    {
        const auto inside = [&](const Math::cVec3f& _rPosition)
        {
            return _rPosition.x() >= _rMinimum.x() && _rPosition.x() < _rMaximum.x()
                && _rPosition.y() >= _rMinimum.y() && _rPosition.y() < _rMaximum.y()
                && _rPosition.z() >= _rMinimum.z() && _rPosition.z() < _rMaximum.z();
        };
        for (uint32_t index = 0; index < m_emitters.size(); ++index)
        {
            if (m_emitters[index].active && inside(m_emitters[index].position))
                StopEmitter({ index, m_emitters[index].generation }, true);
        }
        std::erase_if(m_particles, [&](const sParticle& _rParticle) { return inside(_rParticle.position); });
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cParticleSystem::BeginSurfaces()
    {
        m_surfaces.clear();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cParticleSystem::AddSurface(const sParticleSurface& _rSurface, const std::array<float, 4>& _rColor, float _age)
    {
        if (_rSurface.radius <= 0.0f || m_surfaces.size() == c_maxParticleSurfaces)
            return;

        sParticleData data{};
        data.positionSize = { _rSurface.position.x(), _rSurface.position.y() + 0.04f, _rSurface.position.z(), _rSurface.radius };
        data.color = _rColor;
        data.normalMode = { _rSurface.normal.x(), _rSurface.normal.y(), _rSurface.normal.z(), 1.0f };
        data.rotationAge = { 0.0f, _age, 0.0f, 0.0f };
        m_surfaces.push_back(data);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    std::span<const sParticleData> cParticleSystem::PrepareRender(const Math::cVec3f& _rCameraPosition,
        const Math::cVec3f& _rCameraDirection)
    {
        m_renderData.assign(m_surfaces.begin(), m_surfaces.end());
        for (const sParticle& particle : m_particles)
        {
            const float t = std::clamp(particle.age / particle.definition.lifetime, 0.0f, 1.0f);
            sParticleData data{};
            const float size = particle.definition.startSize * (1.0f - t) + particle.definition.endSize * t;
            data.positionSize = { particle.position.x(), particle.position.y(), particle.position.z(), size };
            for (size_t index = 0; index < 4; ++index)
                data.color[index] = particle.definition.startColor[index] * (1.0f - t) + particle.definition.endColor[index] * t;
            data.color[3] *= std::min(particle.age * 15.0f, 1.0f);
            data.rotationAge = { particle.rotation + particle.age * 0.3f, particle.age, 0.0f, 0.0f };
            m_renderData.push_back(data);
        }
        // Cache depth once; the sort must not reconstruct vectors for every comparison.
        for (sParticleData& data : m_renderData)
        {
            data.rotationAge[2] = (Math::cVec3f(data.positionSize[0], data.positionSize[1], data.positionSize[2])
                - _rCameraPosition).dot(_rCameraDirection);
        }
        std::sort(m_renderData.begin(), m_renderData.end(), [&](const sParticleData& _rLeft, const sParticleData& _rRight)
        {
            return _rLeft.rotationAge[2] > _rRight.rotationAge[2];
        });
        return m_renderData;
    }
}
