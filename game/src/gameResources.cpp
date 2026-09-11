#include "gameInternal.h"

// -------------------------------------------------------------------------------------------------------------------------

void cGame::InitMeshes()
{
    using namespace Engine::GFX;

    sMeshData& planeData    = ShapeMeshLibrary::GetMeshData(sMeshTypes::Plane);
    sMeshData& chunkPlane   = ShapeMeshLibrary::GetMeshData(sMeshTypes::ChunkPlane);
    sMeshData& cubeData     = ShapeMeshLibrary::GetMeshData(sMeshTypes::Cube);
    sMeshData& pyramidData  = ShapeMeshLibrary::GetMeshData(sMeshTypes::Pyramid);
    sMeshData& sphereData   = ShapeMeshLibrary::GetMeshData(sMeshTypes::Sphere);
    sMeshData& cylinderData = ShapeMeshLibrary::GetMeshData(sMeshTypes::Cylinder);
    sMeshData& coneData     = ShapeMeshLibrary::GetMeshData(sMeshTypes::Cone);

    m_planeMesh      = CreateMesh(planeData);
    m_chunkPlaneMesh = CreateMesh(chunkPlane);
    m_cubeMesh       = CreateMesh(cubeData);
    m_pyramidMesh    = CreateMesh(pyramidData);
    m_sphereMesh     = CreateMesh(sphereData);
    m_cylinderMesh   = CreateMesh(cylinderData);
    m_coneMesh       = CreateMesh(coneData);

    m_torusMesh             = CreateMesh(ShapeMeshLibrary::GetMeshData(sMeshTypes::Torus));
    m_crystalMesh           = CreateMesh(ShapeMeshLibrary::GetMeshData(sMeshTypes::Crystal));
    m_beveledCubeMesh       = CreateMesh(ShapeMeshLibrary::GetMeshData(sMeshTypes::BeveledCube));
    m_frustumMesh           = CreateMesh(ShapeMeshLibrary::GetMeshData(sMeshTypes::Frustum));
    m_wedgeMesh             = CreateMesh(ShapeMeshLibrary::GetMeshData(sMeshTypes::Wedge));
    m_triangularPrismMesh   = CreateMesh(ShapeMeshLibrary::GetMeshData(sMeshTypes::TriangularPrism));
    m_icoSphereMesh         = CreateMesh(ShapeMeshLibrary::GetMeshData(sMeshTypes::IcoSphere));
    m_rockMesh              = CreateMesh(ShapeMeshLibrary::GetMeshData(sMeshTypes::Rock));
    m_grassBladeMesh        = CreateMesh(ShapeMeshLibrary::GetMeshData(sMeshTypes::GrassBlade));
    m_capsuleMesh           = CreateMesh(ShapeMeshLibrary::GetMeshData(sMeshTypes::Capsule));
    m_archMesh              = CreateMesh(ShapeMeshLibrary::GetMeshData(sMeshTypes::Arch));
    m_extrudedPolygonMesh   = CreateMesh(ShapeMeshLibrary::GetMeshData(sMeshTypes::ExtrudedPolygon));
    m_discMesh              = CreateMesh(ShapeMeshLibrary::GetMeshData(sMeshTypes::Disc));
    m_arcMesh               = CreateMesh(ShapeMeshLibrary::GetMeshData(sMeshTypes::Arc));

    SubmitMesh(m_planeMesh);
    SubmitMesh(m_chunkPlaneMesh);
    SubmitMesh(m_cubeMesh);
    SubmitMesh(m_pyramidMesh);
    SubmitMesh(m_sphereMesh);
    SubmitMesh(m_cylinderMesh);
    SubmitMesh(m_coneMesh);
    SubmitMesh(m_torusMesh);
    SubmitMesh(m_crystalMesh);
    SubmitMesh(m_beveledCubeMesh);
    SubmitMesh(m_frustumMesh);
    SubmitMesh(m_wedgeMesh);
    SubmitMesh(m_triangularPrismMesh);
    SubmitMesh(m_icoSphereMesh);
    SubmitMesh(m_rockMesh);
    SubmitMesh(m_grassBladeMesh);
    SubmitMesh(m_capsuleMesh);
    SubmitMesh(m_archMesh);
    SubmitMesh(m_extrudedPolygonMesh);
    SubmitMesh(m_discMesh);
    SubmitMesh(m_arcMesh);

    sMaterial playerSphereMaterial{};

    playerSphereMaterial.roughness        = 0.24f;
    playerSphereMaterial.lightWrap        = 1.0f;
    playerSphereMaterial.ambientStrength  = 0.0f;
    playerSphereMaterial.emissiveColor    = { 0.34f, 0.12f, 1.0f };
    playerSphereMaterial.emissiveStrength = 3.2f;

    m_playerSphereMaterial = MaterialManager::CreateMaterial(playerSphereMaterial);

    sLight directionalLight0{};

    directionalLight0.type          = sLightType::Directional;
    directionalLight0.color         = { World::c_moonRed, World::c_moonGreen, World::c_moonBlue };
    directionalLight0.intensity     = World::c_moonIntensity;
    directionalLight0.direction     = { -World::c_moonDirectionX, -World::c_moonDirectionY, -World::c_moonDirectionZ };
    directionalLight0.castsShadow   = true;

    LightManager::CreateLight(directionalLight0);
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::InitNightSky()
{
    using namespace Engine::GFX;

    // An inward-facing cube follows the camera in the shader; the application owns its GPU mesh.
    sMeshData skyMesh = ShapeMeshLibrary::GetMeshData(sMeshTypes::Cube);
    skyMesh.pDebugName = "Geometric night sky";

    for (size_t i = 0; i < skyMesh.indices.size(); i += 3)
        std::swap(skyMesh.indices[i + 1], skyMesh.indices[i + 2]);

    MeshHandle mesh = CreateMesh(skyMesh);
    SubmitMesh(mesh);

    sInstanceData* pInstance = m_pool.Create();
    pInstance->worldMatrix = Engine::Math::cMatrix4x4f::identity();
    pInstance->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    pInstance->materialIndex = -1;
    pInstance->instanceFlags = sInstanceFlags::InstanceFlagSky;

    m_dynamicMeshInstances[mesh].push_back(pInstance);
}

// -------------------------------------------------------------------------------------------------------------------------

bool cGame::LoadPlayerModel()
{
    std::string errorMessage;

    if (!GFX::ShapeModelLoader::LoadFromFile("./assets/models/player_wizard.json", m_playerModel, errorMessage))
    {
        std::cerr << "Failed to load player model: " << errorMessage << "\n";
        return false;
    }

    LoadPoseModel("./assets/models/player_wizard_attack.json", m_playerModel, m_playerAttackModel);
    return true;
}

// -------------------------------------------------------------------------------------------------------------------------

bool cGame::LoadEnemyModels()
{
    std::string errorMessage;

    if (!GFX::ShapeModelLoader::LoadFromFile("./assets/models/enemy_03.json", m_enemy03Model, errorMessage))
    {
        std::cerr << "Failed to load enemy_03: " << errorMessage << '\n';
        return false;
    }

    if (!GFX::ShapeModelLoader::LoadFromFile("./assets/models/enemy_04.json", m_enemy04Model, errorMessage))
    {
        std::cerr << "Failed to load enemy_04: " << errorMessage << '\n';
        return false;
    }

    if (!GFX::ShapeModelLoader::LoadFromFile("./assets/models/forest_thornwolf.json", m_thornwolfModel, errorMessage))
    {
        std::cerr << "Failed to load forest_thornwolf: " << errorMessage << '\n';
        return false;
    }

    LoadPoseModel("./assets/models/forest_thornwolf_attack.json", m_thornwolfModel, m_thornwolfAttackModel);

    if (!GFX::ShapeModelLoader::LoadFromFile("./assets/models/forest_sporecap.json", m_sporecapModel, errorMessage))
    {
        std::cerr << "Failed to load forest_sporecap: " << errorMessage << '\n';
        return false;
    }

    LoadPoseModel("./assets/models/forest_sporecap_attack.json", m_sporecapModel, m_sporecapAttackModel);

    if (!GFX::ShapeModelLoader::LoadFromFile("./assets/models/forest_thornshooter.json", m_thornshooterModel, errorMessage))
    {
        std::cerr << "Failed to load forest_thornshooter: " << errorMessage << '\n';
        return false;
    }

    LoadPoseModel("./assets/models/forest_thornshooter_attack.json", m_thornshooterModel, m_thornshooterAttackModel);

    if (!GFX::ShapeModelLoader::LoadFromFile("./assets/models/forest_rootcharger.json", m_rootchargerModel, errorMessage))
    {
        std::cerr << "Failed to load forest_rootcharger: " << errorMessage << '\n';
        return false;
    }

    LoadPoseModel("./assets/models/forest_rootcharger_attack.json", m_rootchargerModel, m_rootchargerAttackModel);

    if (!GFX::ShapeModelLoader::LoadFromFile("./assets/models/forest_barkguard.json", m_barkguardModel, errorMessage))
    {
        std::cerr << "Failed to load forest_barkguard: " << errorMessage << '\n';
        return false;
    }

    LoadPoseModel("./assets/models/forest_barkguard_attack.json", m_barkguardModel, m_barkguardAttackModel);

    LoadPoseModel("./assets/models/enemy_03_attack.json", m_enemy03Model, m_enemy03AttackModel);
    LoadPoseModel("./assets/models/enemy_04_attack.json", m_enemy04Model, m_enemy04AttackModel);
    return true;
}

// -------------------------------------------------------------------------------------------------------------------------

bool cGame::LoadPoseModel(const char* _pFilePath, const GFX::sShapeModelDesc& _rBaseModel, GFX::sShapeModelDesc& _rPoseModel)
{
    std::string errorMessage;
    if (!GFX::ShapeModelLoader::LoadFromFile(_pFilePath, _rPoseModel, errorMessage))
        return false;

    if (_rPoseModel.shapes.size() != _rBaseModel.shapes.size())
    {
        std::cerr << "Pose model has a different shape count: " << _pFilePath << '\n';
        _rPoseModel = {};
        return false;
    }

    for (size_t shapeIndex = 0; shapeIndex < _rBaseModel.shapes.size(); ++shapeIndex)
    {
        if (_rPoseModel.shapes[shapeIndex].meshType != _rBaseModel.shapes[shapeIndex].meshType)
        {
            std::cerr << "Pose model has a different mesh type at shape " << shapeIndex << ": " << _pFilePath << '\n';
            _rPoseModel = {};
            return false;
        }
    }

    if (_rPoseModel.lights.size() != _rBaseModel.lights.size())
    {
        std::cerr << "Pose model has a different light count: " << _pFilePath << '\n';
        _rPoseModel = {};
        return false;
    }

    for (size_t lightIndex = 0; lightIndex < _rBaseModel.lights.size(); ++lightIndex)
    {
        const GFX::sShapeLightDesc& baseLight = _rBaseModel.lights[lightIndex];
        const GFX::sShapeLightDesc& poseLight = _rPoseModel.lights[lightIndex];

        if (poseLight.name != baseLight.name || poseLight.type != baseLight.type)
        {
            std::cerr << "Pose model has a different light at index " << lightIndex << ": " << _pFilePath << '\n';
            _rPoseModel = {};
            return false;
        }
    }

    return true;
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::SpawnEnemies(const std::vector<World::sEnemySpawn>& _rSpawns, const std::pair<int, int>& _rChunk)
{
    for (const World::sEnemySpawn& spawn : _rSpawns)
    {
        const GFX::sShapeModelDesc* pModel = nullptr;

        switch (spawn.type)
        {
        case World::sEnemyType::ForestCrawler:
            pModel = &m_enemy03Model;
            break;

        case World::sEnemyType::ForestBrute:
            pModel = &m_enemy04Model;
            break;

        case World::sEnemyType::ForestThornwolf:
            pModel = &m_thornwolfModel;
            break;

        case World::sEnemyType::ForestSporecap:
            pModel = &m_sporecapModel;
            break;

        case World::sEnemyType::ForestThornshooter:
            pModel = &m_thornshooterModel;
            break;

        case World::sEnemyType::ForestRootcharger:
            pModel = &m_rootchargerModel;
            break;

        case World::sEnemyType::ForestBarkguard:
            pModel = &m_barkguardModel;
            break;
        }

        if (pModel == nullptr)
            continue;

        sEnemyVisual visual{};
        visual.chunk  = _rChunk;
        visual.pModel = pModel;

        const auto key = std::make_tuple(spawn.position.x(), spawn.position.y(), spawn.position.z());

        auto [entry, inserted] = m_worldEnemies.try_emplace(key);

        if (inserted)
        {
            entry->second = m_enemyManager.Spawn(spawn.type, spawn.position, spawn.rotation, spawn.isBoss, spawn.bossId);
            if (spawn.isBoss && spawn.bossId != World::sBossId::Undefined)
                m_bossHandles.push_back(entry->second);
        }

        visual.handle = entry->second;

        m_enemyManager.SetActive(visual.handle, true);

        visual.previousPosition = m_enemyManager.TryGetEnemy(visual.handle)->position;
        visual.renderParts.reserve(pModel->shapes.size());

        for (const GFX::sShapePartDesc& part : pModel->shapes)
        {
            GFX::sInstanceData* pInstance = m_pool.Create();

            pInstance->color =
            {
                part.color[0],
                part.color[1],
                part.color[2],
                part.color[3]
            };

            pInstance->materialIndex = part.materialIndex;
            pInstance->instanceFlags |= GFX::sInstanceFlags::InstanceFlagPreserveAtDistance;

            GFX::MeshHandle mesh = GetMesh(part.meshType);

            m_dynamicMeshInstances[mesh].push_back(pInstance);

            sEnemyRenderPart renderPart{};

            renderPart.pInstance = pInstance;
            renderPart.transform = part.transform;

            visual.renderParts.push_back(renderPart);
        }

        const Gameplay::sEnemy* pEnemy = m_enemyManager.TryGetEnemy(visual.handle);

        if (pEnemy != nullptr)
        {
            GFX::sTransform enemyTransform{};
            enemyTransform.position = pEnemy->position;
            enemyTransform.rotation = { 0.0f, pEnemy->rotation, 0.0f };
            enemyTransform.scale    = { pEnemy->scale, pEnemy->scale, pEnemy->scale };

            GFX::ShapeModelLights::Create(*pModel, enemyTransform, visual.lightHandles);
        }

        m_enemyVisuals.push_back(std::move(visual));
    }
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::UpdateFreeCam(float _deltaTime)
{
    using namespace Engine::Platform;

    Engine::GFX::cCamera& rCamera = Engine::GFX::GetCamera();

    constexpr float moveSpeed = 10.0f;
    constexpr float rotationSpeed = 100.0f;

    if (IsKeyDown(c_downArrowKey))
        rCamera.AddPitch(-rotationSpeed * _deltaTime);

    if (IsKeyDown(c_upArrowKey))
        rCamera.AddPitch(rotationSpeed * _deltaTime);

    if (IsKeyDown(c_leftArrowKey))
        rCamera.AddYaw(-rotationSpeed * _deltaTime);

    if (IsKeyDown(c_rightArrowKey))
        rCamera.AddYaw(rotationSpeed * _deltaTime);

    float direction[4];
    rCamera.GetDirection(direction);

    Engine::Math::cVec3f forward(direction[0], direction[1], direction[2]);
    forward.normalize();

    Engine::Math::cVec3f right(-forward.z(), 0.0f, forward.x());
    right.normalize();

    Engine::Math::cVec3f movement;

    if (IsKeyDown('W'))
        movement += forward * moveSpeed * _deltaTime;

    if (IsKeyDown('S'))
        movement -= forward * moveSpeed * _deltaTime;

    if (IsKeyDown('A'))
        movement -= right * moveSpeed * _deltaTime;

    if (IsKeyDown('D'))
        movement += right * moveSpeed * _deltaTime;

    if (IsKeyDown('Q'))
        movement += Engine::Math::cVec3f(0.0f, -moveSpeed * _deltaTime, 0.0f);

    if (IsKeyDown('E'))
        movement += Engine::Math::cVec3f(0.0f, moveSpeed * _deltaTime, 0.0f);

    if (movement.isZero())
        return;

    float position[4];
    rCamera.GetPosition(position);

    Engine::Math::cVec3f cameraPosition(position[0], position[1], position[2]);

    cameraPosition += movement;

    rCamera.SetPosition(cameraPosition.x(), cameraPosition.y(), cameraPosition.z());
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::RefreshWorldRenderInstances()
{
    const auto& chunks = World::WorldGenerator::GetLoadedChunks();
    std::unordered_set<GFX::sInstanceData*> removed;

    std::erase_if(m_worldRenderInstances, [&](auto& _rEntry)
    {
        if (chunks.contains(_rEntry.first))
            return false;

        const float minimumX = (_rEntry.first.first - 0.5f) * World::c_chunkSize;
        const float minimumZ = (_rEntry.first.second - 0.5f) * World::c_chunkSize;

        const float heightLimit = std::numeric_limits<float>::max();

        m_particleSystem.RemoveInBounds({ minimumX, -heightLimit, minimumZ },
            { minimumX + World::c_chunkSize, heightLimit, minimumZ + World::c_chunkSize });

        GFX::ShapeModelLights::Destroy(_rEntry.second.lightHandles);

        for (GFX::ReflectionProbeHandle probeHandle : _rEntry.second.reflectionProbeHandles)
            GFX::ReflectionProbeManager::RemoveProbe(probeHandle);

        return true;
    });

    std::erase_if(m_enemyVisuals, [&](auto& _rVisual)
    {
        if (chunks.contains(_rVisual.chunk))
            return false;

        m_enemyManager.SetActive(_rVisual.handle, false);
        for (const auto& part : _rVisual.renderParts)
            removed.insert(part.pInstance);

        GFX::ShapeModelLights::Destroy(_rVisual.lightHandles);

        return true;
    });

    if (!removed.empty())
    {
        for (auto& [mesh, instances] : m_dynamicMeshInstances)
            std::erase_if(instances, [&](auto* _pInstance) { return removed.contains(_pInstance); });

        for (auto* pInstance : removed)
            m_pool.Destroy(pInstance);
    }

    for (const auto& [coordinate, chunk] : chunks)
    {
        auto [entry, inserted] = m_worldRenderInstances.try_emplace(coordinate);
        if (!inserted)
            continue;

        const float limit = std::numeric_limits<float>::max();

        entry->second.bounds.min = { limit, limit, limit };
        entry->second.bounds.max = { -limit, -limit, -limit };

        for (const auto& shape : chunk.scene.GetShapeInstances())
            BuildRenderInstances(shape, entry->second);

        if (entry->second.meshInstances.empty())
        {
            entry->second.bounds = {};
        }
        else
        {
            entry->second.bounds.center = (entry->second.bounds.min + entry->second.bounds.max) * 0.5f;
            entry->second.bounds.size   = entry->second.bounds.max  - entry->second.bounds.min;
            entry->second.bounds.radius = entry->second.bounds.size.length() * 0.5f;
        }

        for (const World::sReflectionProbeDesc& description : chunk.reflectionProbes)
        {
            GFX::sReflectionProbe probe{};
            probe.position       = description.position;
            probe.boxMin         = description.boxMin;
            probe.boxMax         = description.boxMax;
            probe.blendDistance  = description.blendDistance;
            probe.resolution     = description.resolution;
            probe.projectionType = GFX::sReflectionProbeProjectionType::Box;

            entry->second.reflectionProbeHandles.push_back(GFX::ReflectionProbeManager::AddProbe(probe));
        }

        if (m_enemyModelsLoaded)
            SpawnEnemies(chunk.spawns, coordinate);
    }

    RebuildWorldInstanceList();
    m_dynamicInstanceListDirty = true;
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::BuildRenderInstances(const GFX::sShapeInstance& _rShapeInstance, sWorldRenderInstances& _rInstances)
{
    using namespace Engine::GFX;
    using namespace Engine::Math;

    const sShapeModelDesc& model = ShapeModelManager::GetShapeModel(_rShapeInstance.modelHandle);

    cMatrix4x4f instanceMatrix = CreateTransformMatrix(_rShapeInstance.transform);

    for (const sShapePartDesc& part : model.shapes)
    {
        sInstanceData instance{};

        cMatrix4x4f partMatrix = CreateTransformMatrix(part.transform);

        instance.worldMatrix = partMatrix * instanceMatrix;

        instance.color =
        {
            part.color[0],
            part.color[1],
            part.color[2],
            part.color[3]
        };

        instance.materialIndex = part.materialIndex;
        instance.instanceFlags |= sInstanceFlags::InstanceFlagPreserveAtDistance;

        if (model.pDebugName == "rock" || model.pDebugName.starts_with("rock_"))
        {
            instance.instanceFlags |= sInstanceFlags::InstanceFlagWeathered;
        }

        MeshHandle mesh = GetMesh(part.meshType);

        if (part.meshType == sMeshTypes::ChunkPlane)
        {
            instance.instanceFlags |= sInstanceFlags::InstanceFlagTerrain;
        }

        if (part.meshType == sMeshTypes::Crystal)
        {
            instance.instanceFlags |= sInstanceFlags::InstanceFlagCrystal;
        }

        const auto extendBounds = [&](const cVec3f& _rPoint)
        {
            _rInstances.bounds.min =
            {
                std::min(_rInstances.bounds.min.x(), _rPoint.x()),
                std::min(_rInstances.bounds.min.y(), _rPoint.y()),
                std::min(_rInstances.bounds.min.z(), _rPoint.z())
            };
            _rInstances.bounds.max =
            {
                std::max(_rInstances.bounds.max.x(), _rPoint.x()),
                std::max(_rInstances.bounds.max.y(), _rPoint.y()),
                std::max(_rInstances.bounds.max.z(), _rPoint.z())
            };
        };

        if (part.meshType == sMeshTypes::ChunkPlane)
        {
            // Terrain is displaced after the world transform in every vertex pass.
            for (const auto& vertex : ShapeMeshLibrary::GetMeshData(part.meshType).vertices)
            {
                cVec3f position = instance.worldMatrix.transformPoint(vertex.position);
                position += cVec3f(0.0f, World::GetTerrainHeight(position.x(), position.z()), 0.0f);
                extendBounds(position);
            }
        }
        else
        {
            const sBounds& bounds = ShapeMeshLibrary::GetBounds(part.meshType);
            for (uint32_t corner = 0; corner < 8; ++corner)
            {
                extendBounds(instance.worldMatrix.transformPoint(
                {
                    (corner & 1) != 0 ? bounds.max.x() : bounds.min.x(),
                    (corner & 2) != 0 ? bounds.max.y() : bounds.min.y(),
                    (corner & 4) != 0 ? bounds.max.z() : bounds.min.z()
                }));
            }
        }

        _rInstances.meshInstances[mesh].push_back(instance);
    }

    if (_rShapeInstance.generateLights)
    {
        std::vector<LightHandle> lightHandles;
        ShapeModelLights::Create(model, _rShapeInstance.transform, lightHandles);
        _rInstances.lightHandles.insert(_rInstances.lightHandles.end(), lightHandles.begin(), lightHandles.end());
    }
}

// -------------------------------------------------------------------------------------------------------------------------


void cGame::BuildPlayerRenderInstances()
{
    using namespace Engine::GFX;

    m_playerRenderParts.clear();
    m_playerRenderParts.reserve(m_playerModel.shapes.size());

    for (const sShapePartDesc& rPart : m_playerModel.shapes)
    {
        sInstanceData* pInstance = m_pool.Create();

        pInstance->color =
        {
            rPart.color[0],
            rPart.color[1],
            rPart.color[2],
            rPart.color[3]
        };

        pInstance->materialIndex = rPart.materialIndex;

        MeshHandle mesh = GetMesh(rPart.meshType);

        m_dynamicMeshInstances[mesh].push_back(pInstance);

        sPlayerRenderPart renderPart{};

        renderPart.pInstance = pInstance;
        renderPart.transform = rPart.transform;

        m_playerRenderParts.push_back(renderPart);
    }

    sTransform playerTransform{};
    playerTransform.position = m_playerController.GetPosition();
    playerTransform.rotation = { 0.0f, m_playerYaw, 0.0f };
    playerTransform.scale    = { 1.0f, 1.0f, 1.0f };

    ShapeModelLights::Create(m_playerModel, playerTransform, m_playerLightHandles);
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::RebuildDynamicInstanceList()
{
    m_dynamicInstances.clear();

    for (auto& [mesh, instances] : m_dynamicMeshInstances)
    {
        for (Engine::GFX::sInstanceData* pInstance : instances)
            m_dynamicInstances.push_back(pInstance);
    }

    m_dynamicInstanceListDirty = false;
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::RebuildWorldInstanceList()
{
    m_worldDrawBatches.clear();
    size_t instanceCount = 0;

    for (const auto& [coordinate, chunk] : m_worldRenderInstances)
    {
        for (const auto& [mesh, instances] : chunk.meshInstances)
        {
            if (instances.empty())
                continue;

            m_worldDrawBatches.push_back({ mesh, &chunk, 0, static_cast<uint32_t>(instances.size()) });
            instanceCount += instances.size();
        }
    }

    // Adjacent visible chunks sharing a mesh can still use a single instanced draw.
    std::stable_sort(m_worldDrawBatches.begin(), m_worldDrawBatches.end(), [](const auto& _rLeft, const auto& _rRight)
    {
        return std::less<GFX::MeshHandle>{}(_rLeft.mesh, _rRight.mesh);
    });

    m_staticInstances.clear();
    m_staticInstances.reserve(instanceCount);

    for (sWorldDrawBatch& batch : m_worldDrawBatches)
    {
        batch.firstInstance = static_cast<uint32_t>(m_staticInstances.size());
        const auto& instances = batch.pChunk->meshInstances.at(batch.mesh);
        m_staticInstances.insert(m_staticInstances.end(), instances.begin(), instances.end());
    }

    ++m_staticInstanceRevision;
}

// -------------------------------------------------------------------------------------------------------------------------

void cGame::ClearRenderInstances()
{
    for (const auto& [mesh, instances] : m_dynamicMeshInstances)
    {
        for (GFX::sInstanceData* pInstance : instances)
            m_pool.Destroy(pInstance);
    }

    m_dynamicInstances.clear();
    m_dynamicMeshInstances.clear();
    m_staticInstances.clear();
    m_worldDrawBatches.clear();
    ++m_staticInstanceRevision;
}

// -------------------------------------------------------------------------------------------------------------------------

