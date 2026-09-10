# Particle effects

The mushroom spell uses CPU-simulated spores and instanced transparent quads. Gameplay owns projectile movement, impact, poison duration and damage; the engine only receives visual definitions and world-space surfaces.

## Integration

- `engine/src/graphics/particles/particleSystem.*`: emitter handles, fixed-capacity storage, lifetime integration, bursts, surface emission and depth sorting.
- `particleData.h` and `game/assets/shaders/particles.hlsl`: shared four-float4 instance layout. Camera-facing spores and ground-oriented patches use the same premultiplied-alpha pipeline.
- `cGame::UpdateProjectileEffects`: binds projectile IDs to emitters, consumes explicit impact events, submits ground markers and changes trail emitters into area emitters.
- `cProjectileManager::Update`: impact events survive projectile removal during a long frame. Consumers clear the events after processing; the next update also clears old events.
- `OnUpdate` simulates once; `OnPrepareRender` sorts and uploads once. The application draws particles after opaque geometry and before health bars. Shadow, AO and reflection captures omit particles.

## Creating another effect

Set a `sParticleDefinition` (rate, lifetime, velocity spread, acceleration, size and color endpoints), call `CreateEmitter`, and keep its generation-checked handle. Call `SetPosition` to move it or `SetSurfaces` to emit above supplied surfaces. `Burst` creates a one-shot effect without an emitter.

`StopEmitter(handle)` allows existing particles to fade. Pass `true` to delete owned particles immediately. `RemoveInBounds` removes both emitters and remaining particles when a region unloads; `Clear` invalidates all handles for shutdown/reset.

Each frame, call `BeginSurfaces`, then `AddSurface` for active ground markers. The mushroom passes the same sampled positions, normals and growing patch radii that its damage checks use. Marker submission stops immediately when the area expires. Floating spores can fade afterward. Player poison is teal-green; enemy poison is yellow-green.

## Budgets and rendering limits

Storage is reserved at construction: 8,192 moving particles, 256 emitters, up to 169 samples per emitter, and 32,768 ground-marker quads independently of the moving-particle budget. At capacity, new visual particles are dropped; damage is unaffected. `GetDroppedParticleCount` exposes dropped births. Ground-marker submissions above their separate limit are omitted.

Each frame-in-flight has its own mapped vertex buffer. One instanced draw uses depth testing without depth writes, sorted back to front. CPU sort keys are computed once per instance. `GetParticleGpuMilliseconds` returns the last completed GPU timing, or a negative value when unavailable.

The first version uses procedural shapes and ordinary depth testing. It does not provide depth-buffer soft intersections, GPU simulation, particle collisions, an effect editor or shadow/reflection participation. Ground geometry is approximated by the existing samples; this is not a terrain decal tessellator.

## Validation

Generate projects after adding source files, build Debug engine, compile shaders with `scripts/compileShaders.bat`, and build Debug game to refresh runtime assets.

- `scripts/checkParticles.bat`: standalone CPU/gameplay assertions, including handle reuse, limits, pause, unload cleanup, frame partitions, slope/step damage, team filtering, long-frame impact events and boss reward mapping. The 20-field loop checks that simulation and render preparation allocate no memory per frame.
- `scripts/checkParticles.bat render`: standalone 300-frame Vulkan fixture with 20 overlapping fields, sloped/stepped geometry, camera movement and window resizing. It opens a test window and exits automatically. `render-build` only compiles it.

The runner requires the existing Windows Visual Studio/Vulkan/vcpkg build setup. Test executables and captures are written under `%TEMP%/CPE-particle-checks`; no new Premake test target or library dependency is introduced.

Validated with the Debug build on an RTX 4070 Ti: CPU assertions and shader compilation passed; the Vulkan fixture completed without validation messages. The final fixture run averaged approximately 2.1 ms for CPU simulation/preparation/upload and 0.085 ms for the GPU particle section. The separate CPU assertion fixture averaged 1.1 ms, with 2,360 moving particles and 2,260 marker quads. These are synthetic fixture measurements, not full-game frame times or a release benchmark. Captures were inspected; combat balance has not been manually playtested.
