#pragma once

#include "particleData.h"

#include <span>
#include <vector>

namespace Engine::GFX
{
    class cParticleSystem
    {
        public:

            cParticleSystem();
            sParticleEmitterHandle CreateEmitter(const sParticleDefinition& _rDefinition, const Math::cVec3f& _rPosition);
            bool IsAlive(sParticleEmitterHandle _handle) const;
            void SetPosition(sParticleEmitterHandle _handle, const Math::cVec3f& _rPosition);
            void SetSurfaces(sParticleEmitterHandle _handle, std::span<const sParticleSurface> _surfaces);
            void StopEmitter(sParticleEmitterHandle _handle, bool _clearParticles = false);
            void Burst(const sParticleDefinition& _rDefinition, const Math::cVec3f& _rPosition, uint32_t _count);
            void Update(float _deltaTime);
            void Clear();
            void RemoveInBounds(const Math::cVec3f& _rMinimum, const Math::cVec3f& _rMaximum);

            void BeginSurfaces();
            void AddSurface(const sParticleSurface& _rSurface, const std::array<float, 4>& _rColor, float _age);
            std::span<const sParticleData> PrepareRender(const Math::cVec3f& _rCameraPosition, const Math::cVec3f& _rCameraDirection);
            size_t GetParticleCount() const { return m_particles.size(); }
            uint64_t GetDroppedParticleCount() const { return m_droppedParticles; }

        private:

            static constexpr size_t c_maxEmitters = 256;
            static constexpr size_t c_maxEmitterSurfaces = 169;

            struct sEmitter
            {
                sParticleDefinition definition;
                Math::cVec3f position{};
                Math::cVec3f previousPosition{};
                std::array<sParticleSurface, c_maxEmitterSurfaces> surfaces{};
                size_t surfaceCount = 0;
                uint32_t generation = 1;
                float spawnRemainder = 0.0f;
                bool active = false;
            };

            struct sParticle
            {
                sParticleDefinition definition;
                sParticleEmitterHandle owner;
                Math::cVec3f position{};
                Math::cVec3f velocity{};
                float age = 0.0f;
                float rotation = 0.0f;
            };

            float Random();
            void Emit(const sParticleDefinition& _rDefinition, const Math::cVec3f& _rPosition,
                const Math::cVec3f& _rNormal, sParticleEmitterHandle _owner, float _age);

            std::vector<sEmitter> m_emitters;
            std::vector<sParticle> m_particles;
            std::vector<sParticleData> m_surfaces;
            std::vector<sParticleData> m_renderData;
            uint32_t m_randomState = 0x719ab32d;
            uint64_t m_droppedParticles = 0;
    };
}
