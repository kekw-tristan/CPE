#include "graphics/particles/particleSystem.h"
#include "enemy/projectileManager.h"
#include "spells/spellManager.h"
#include "physics/collisionWorld.h"
#include "physics/collider.h"

#include <cassert>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <new>

namespace
{
    size_t g_allocations = 0;
    float Slope(float _x, float) { return _x * 0.5f; }
}

void* operator new(size_t _size)
{
    ++g_allocations;
    if (void* pMemory = std::malloc(_size == 0 ? 1 : _size))
        return pMemory;
    throw std::bad_alloc();
}
void operator delete(void* _pMemory) noexcept { std::free(_pMemory); }
void operator delete(void* _pMemory, size_t) noexcept { std::free(_pMemory); }

int main()
{
    using namespace Engine;
    using namespace Engine::GFX;
    using namespace Gameplay;

    cParticleSystem particles;
    sParticleDefinition definition{};
    definition.spawnRate = 20.0f;
    definition.lifetime = 2.0f;
    const auto first = particles.CreateEmitter(definition, {});
    particles.Update(0.25f);
    assert(particles.GetParticleCount() == 5);
    particles.StopEmitter(first, true);
    assert(particles.GetParticleCount() == 0 && !particles.IsAlive(first));
    const auto reused = particles.CreateEmitter(definition, {});
    assert(reused.index == first.index && reused.generation != first.generation);
    particles.StopEmitter(first, true);
    assert(particles.IsAlive(reused));
    particles.Update(0.0f);
    assert(particles.GetParticleCount() == 0);
    particles.Clear();
    assert(!particles.IsAlive(reused));

    particles.Burst(definition, {}, static_cast<uint32_t>(c_maxParticles + 200));
    assert(particles.GetParticleCount() == c_maxParticles);
    assert(particles.GetDroppedParticleCount() == 200);
    particles.AddSurface({ { 0.0f, 0.0f, 4.0f }, { 0.0f, 1.0f, 0.0f }, 1.0f }, { 0.5f, 0.8f, 0.1f, 0.5f }, 0.0f);
    auto draws = particles.PrepareRender({}, { 0.0f, 0.0f, 1.0f });
    assert(draws.size() == c_maxParticles + 1 && draws.front().normalMode[3] == 1.0f);
    particles.Update(3.0f);
    assert(particles.GetParticleCount() == 0);
    particles.Clear();

    for (int index = 0; index < 256; ++index)
        assert(particles.IsAlive(particles.CreateEmitter(definition, {})));
    assert(!particles.IsAlive(particles.CreateEmitter(definition, {})));
    particles.Clear();

    cParticleSystem splitFrames;
    particles.CreateEmitter(definition, {});
    splitFrames.CreateEmitter(definition, {});
    particles.Update(1.0f);
    for (int frame = 0; frame < 4; ++frame)
        splitFrames.Update(0.25f);
    assert(particles.GetParticleCount() == splitFrames.GetParticleCount());
    particles.RemoveInBounds({ -1.0f, -1.0f, -1.0f }, { 1.0f, 4.0f, 1.0f });
    assert(particles.GetParticleCount() == 0);
    particles.Update(0.25f);
    assert(particles.GetParticleCount() == 0);
    particles.Clear();

    Physics::sAABBCollider ground{};
    ground.center = { 0.0f, -1.0f, 0.0f };
    ground.halfExtents = { 20.0f, 1.0f, 20.0f };
    ground.isGround = true;
    ground.groundHeightSampler = Slope;
    Physics::CollisionWorld::AddCollider(ground);
    Physics::sAABBCollider step{};
    step.center = { -2.0f, 0.0f, 0.0f };
    step.halfExtents = { 0.6f, 1.0f, 0.6f };
    step.isGround = true;
    Physics::CollisionWorld::AddCollider(step);
    cEnemyManager enemies;
    cProjectileManager shots;
    sProjectileSpawnDesc desc{};
    desc.position = { 0.0f, 3.0f, 0.0f };
    desc.direction = { 0.0f, -1.0f, 0.0f };
    desc.speed = 10.0f;
    desc.damage = 36.0f;
    desc.lifetime = 2.0f;
    desc.isAreaOfEffect = true;
    desc.areaRadius = 4.0f;
    desc.areaDuration = 4.5f;
    desc.areaGrowthTime = 0.9f;
    shots.SpawnSpore(desc);
    shots.Update(1.5f, { 2.0f, 1.0f, 0.0f }, enemies);
    assert(shots.GetImpactEvents().size() == 1);
    const sProjectile& area = shots.GetProjectiles().front();
    assert(area.ContainsGroundPoint({ 2.0f, 1.0f, 0.0f }));
    assert(area.ContainsGroundPoint({ -2.0f, 1.0f, 0.0f }));
    assert(!area.ContainsGroundPoint({ -2.0f, -1.0f, 0.0f }));
    assert(!area.ContainsGroundPoint({ 2.0f, 4.0f, 0.0f }));
    assert(shots.ConsumePlayerDamage() > 0.0f);
    assert(shots.ConsumePlayerDamage() == 0.0f);

    std::array<sParticleSurface, sProjectile::c_maxGroundSamples> surfaces{};
    const size_t sampleCount = area.groundSampleCount;
    for (size_t index = 0; index < sampleCount; ++index)
    {
        surfaces[index] = { area.groundSamples[index], area.groundNormals[index], area.GetGroundSampleRadius(index) };
        surfaces[index].tileHalfExtent = area.areaRadius / 13.0f;
        surfaces[index].areaClip = { area.position.x(), area.position.z(), area.radius };
    }
    definition.spawnRate = 65.0f;
    definition.lifetime = 1.8f;
    definition.speed = 0.18f;
    for (int field = 0; field < 20; ++field)
    {
        const auto emitter = particles.CreateEmitter(definition, {});
        particles.SetSurfaces(emitter, { surfaces.data(), sampleCount });
    }
    const auto start = std::chrono::steady_clock::now();
    const size_t allocationCount = g_allocations;
    for (int frame = 0; frame < 600; ++frame)
    {
        particles.BeginSurfaces();
        for (int field = 0; field < 20; ++field)
        {
            for (size_t index = 0; index < sampleCount; ++index)
                particles.AddSurface(surfaces[index], { 0.5f, 0.8f, 0.1f, 0.5f }, frame / 60.0f);
        }
        particles.Update(1.0f / 60.0f);
        draws = particles.PrepareRender({ 0.0f, 10.0f, -10.0f }, { 0.0f, -0.7f, 0.7f });
        for (size_t index = 1; index < draws.size(); ++index)
            assert(-draws[index - 1].positionSize[1] + draws[index - 1].positionSize[2]
                >= -draws[index].positionSize[1] + draws[index].positionSize[2] - 0.0001f);
    }
    assert(g_allocations == allocationCount);
    const double milliseconds = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count() / 600.0;
    std::cout << "20 fields: " << particles.GetParticleCount() << " particles, " << sampleCount * 20
        << " surfaces, " << milliseconds << " ms/frame (CPU simulation, preparation and sort checks); zero frame allocations.\n";
    shots.Clear();
    shots.SpawnSpore(desc);
    shots.Update(8.0f, { 20.0f, 0.0f, 20.0f }, enemies);
    assert(shots.GetProjectiles().empty() && shots.GetImpactEvents().size() == 1);
    shots.ClearImpactEvents();
    assert(shots.GetImpactEvents().empty());
    auto grounded = enemies.Spawn(World::sEnemyType::ForestSporecap, { 2.0f, 1.0f, 0.0f }, 0.0f);
    auto elevated = enemies.Spawn(World::sEnemyType::ForestSporecap, { 2.0f, 4.0f, 0.0f }, 0.0f);
    const float health = enemies.TryGetEnemy(grounded)->health;
    shots.SpawnPlayerSpore(desc);
    shots.Update(1.5f, { 20.0f, 0.0f, 20.0f }, enemies);
    assert(enemies.TryGetEnemy(grounded)->health < health);
    assert(enemies.TryGetEnemy(elevated)->health == health);
    assert(shots.ConsumePlayerDamage() == 0.0f);
    assert(SpellManager::GetBoss(World::sBossId::ForestSporecap).spellReward == sSpellId::SporeOrb);
    shots.Clear();
    desc.position = { -8.0f, 5.0f, 8.0f };
    desc.speed = 12.0f;
    AimMushroomThrow(desc, { 4.0f, 2.2f, 8.0f });
    assert(desc.gravity > 0.0f && desc.direction.y() > 0.0f);
    shots.SpawnPlayerSpore(desc);
    shots.Update(0.2f, { 20.0f, 0.0f, 20.0f }, enemies);
    assert(!shots.GetProjectiles().front().areaActive);
    assert(shots.GetProjectiles().front().position.y() > 5.0f);
    shots.Update(1.0f, { 20.0f, 0.0f, 20.0f }, enemies);
    const auto landing = shots.GetProjectiles().front().position;
    assert(shots.GetProjectiles().front().areaActive && std::abs(landing.x() - 4.0f) < 0.5f);
    cProjectileManager fineSteps;
    fineSteps.SpawnPlayerSpore(desc);
    for (int frame = 0; frame < 60; ++frame)
        fineSteps.Update(0.02f, { 20.0f, 0.0f, 20.0f }, enemies);
    assert(Engine::Math::cVec3f::distance(landing, fineSteps.GetProjectiles().front().position) < 0.15f);
    std::cout << "PASS: ballistic rise/landing and frame partitions.\n";
    std::cout << "PASS: handles, capacity, pause, cleanup, frame partitions, slopes, steps, teams, long-frame impact, boss reward.\n";
}
