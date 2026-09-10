// Standalone Vulkan fixture: run scripts/checkParticles.bat render after building Debug game.
#include "application.h"
#include "graphics/camera.h"
#include "graphics/instanceData.h"
#include "graphics/material/material.h"
#include "graphics/material/materialManager.h"
#include "graphics/particles/particleSystem.h"
#include "graphics/shapeModel/meshGenerator.h"

#include <Windows.h>
#include <chrono>
#include <cmath>
#include <fstream>
#include <cstdlib>
#include <string>
#include <iostream>
#include <vector>

namespace
{
    struct sFinished {};
    constexpr const char* c_title = "CPE particle render check";

    void Capture(const char* _pName)
    {
        HWND window = FindWindowA(nullptr, c_title);
        RECT rect{};
        GetClientRect(window, &rect);
        HDC source = GetDC(window);
        HDC destination = CreateCompatibleDC(source);
        HBITMAP bitmap = CreateCompatibleBitmap(source, rect.right, rect.bottom);
        HGDIOBJ previous = SelectObject(destination, bitmap);
        BitBlt(destination, 0, 0, rect.right, rect.bottom, source, 0, 0, SRCCOPY);
        SelectObject(destination, previous);
        BITMAPINFO info{};
        info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        info.bmiHeader.biWidth = rect.right;
        info.bmiHeader.biHeight = -rect.bottom;
        info.bmiHeader.biPlanes = 1;
        info.bmiHeader.biBitCount = 32;
        std::vector<char> pixels(static_cast<size_t>(rect.right) * rect.bottom * 4);
        GetDIBits(destination, bitmap, 0, rect.bottom, pixels.data(), &info, DIB_RGB_COLORS);
        BITMAPFILEHEADER header{};
        header.bfType = 0x4d42;
        header.bfOffBits = sizeof(header) + sizeof(BITMAPINFOHEADER);
        header.bfSize = header.bfOffBits + static_cast<DWORD>(pixels.size());
        std::ofstream file(std::string(std::getenv("TEMP")) + "/CPE-particle-checks/" + _pName, std::ios::binary);
        file.write(reinterpret_cast<const char*>(&header), sizeof(header));
        file.write(reinterpret_cast<const char*>(&info.bmiHeader), sizeof(BITMAPINFOHEADER));
        file.write(pixels.data(), pixels.size());
        DeleteObject(bitmap);
        DeleteDC(destination);
        ReleaseDC(window, source);
    }

    class cParticleRenderCheck : public Engine::cApplication
    {
        public:

            explicit cParticleRenderCheck(Engine::sAppConfig& _rConfig) : cApplication(_rConfig) {}

        private:

            void OnInit() override
            {
                using namespace Engine::GFX;
                Engine::Platform::SetMouseCaptured(false);
                sMaterial material{};
                material.albedo = { 0.3f, 0.32f, 0.34f };
                material.emissiveColor = { 0.13f, 0.14f, 0.15f };
                material.emissiveStrength = 1.0f;
                m_instance.materialIndex = MaterialManager::CreateMaterial(material);
                sMeshData mesh = cMeshGenerator::CreatePlane({ 40.0f, 40.0f, 80, 80 });
                for (auto& vertex : mesh.vertices)
                {
                    const float x = vertex.position.x();
                    const float z = vertex.position.z();
                    vertex.position = { x, Height(x, z), z };
                    vertex.normal = Engine::Math::cVec3f(-0.25f, 1.0f, 0.0f).normalized();
                }
                mesh.bounds.min = { -20.0f, 40.0f, -20.0f };
                mesh.bounds.max = { 20.0f, 58.0f, 20.0f };
                mesh.bounds.center = { 0.0f, 49.0f, 0.0f };
                mesh.bounds.radius = 32.0f;
                m_mesh = CreateMesh(mesh);
                SubmitMesh(m_mesh);
                m_instance.worldMatrix = Engine::Math::cMatrix4x4f::identity();
                m_instance.color = { 0.6f, 0.65f, 0.7f, 1.0f };
                m_instances.push_back(&m_instance);
                for (int field = 0; field < 20; ++field)
                {
                    const float centerX = (field % 5 - 2) * 3.5f;
                    const float centerZ = (field / 5 - 1.5f) * 3.5f;
                    std::array<sParticleSurface, 169> surfaces{};
                    size_t count = 0;
                    for (int z = -6; z <= 6; ++z)
                    {
                        for (int x = -6; x <= 6; ++x)
                        {
                            const float worldX = centerX + x * 4.0f / 6.5f;
                            const float worldZ = centerZ + z * 4.0f / 6.5f;
                            surfaces[count] = { { worldX, Height(worldX, worldZ), worldZ },
                                Engine::Math::cVec3f(-0.25f, 1.0f, 0.0f).normalized(),
                                std::min(0.46f, std::max(0.0f, 4.0f - std::sqrt(float(x * x + z * z)) * 4.0f / 6.5f)) };
                            surfaces[count].tileHalfExtent = 4.0f / 13.0f;
                            surfaces[count].areaClip = { centerX, centerZ, 4.0f };
                            ++count;
                        }
                    }
                    sParticleDefinition definition{};
                    definition.appearance = eParticleAppearance::Vapor;
                    definition.spawnRate = 75.0f;
                    definition.lifetime = 2.0f;
                    definition.speed = 0.13f;
                    definition.startSize = 0.35f;
                    definition.endSize = 0.85f;
                    definition.startColor = field % 2 ? std::array<float, 4>{ 0.12f, 0.85f, 0.38f, 0.32f }
                        : std::array<float, 4>{ 0.4f, 0.9f, 0.015f, 0.32f };
                    definition.endColor = definition.startColor;
                    definition.endColor[3] = 0.0f;
                    const auto emitter = m_particles.CreateEmitter(definition, { centerX, Height(centerX, centerZ), centerZ });
                    m_particles.SetSurfaces(emitter, { surfaces.data(), count });
                    for (size_t index = 0; index < count; ++index)
                        m_surfaces.push_back(surfaces[index]);
                }
                GetCamera().SetPerspective(60.0f, 0.1f, 200.0f);
            }

            static float Height(float _x, float _z)
            {
                return 48.0f + _x * 0.25f + (_x >= 0.0f && _z < 0.0f ? 2.0f : 0.0f);
            }

            void OnUpdate(float) override
            {
                using namespace Engine::GFX;
                ++m_frame;
                if (m_frame == 45) Capture("particle-single.bmp");
                if (m_frame == 90) Capture("particle-overview.bmp");
                if (m_frame == 180) Capture("particle-slope.bmp");
                if (m_frame == 270) Capture("particle-near.bmp");
                if (m_frame == 100)
                    SetWindowPos(FindWindowA(nullptr, c_title), nullptr, 0, 0, 1100, 700, SWP_NOMOVE | SWP_NOZORDER);
                if (m_frame > 300)
                {
                    std::cout << "PASS: 300 Vulkan frames, 20 overlapping fields, resize, slope, step and close camera.\n"
                        << "Particle GPU mean: " << m_gpuSum / m_gpuSamples << " ms; CPU update/upload preparation mean: "
                        << m_cpuSum / 300.0 << " ms.\n";
                    throw sFinished{};
                }
                const float angle = m_frame * 0.01f;
                if (m_frame < 100)
                    GetCamera().LookAt(16.0f, 65.0f, 20.0f, 0.0f, 48.0f, 0.0f);
                else if (m_frame < 200)
                    GetCamera().LookAt(std::sin(angle) * 18.0f, 55.0f, std::cos(angle) * 18.0f, 0.0f, 49.0f, 0.0f);
                else
                    GetCamera().LookAt(-2.0f, 48.5f, 3.0f, 5.0f, 50.0f, -4.0f);
                const auto start = std::chrono::steady_clock::now();
                m_particles.BeginSurfaces();
                for (size_t index = 0; index < m_surfaces.size(); ++index)
                {
                    if (m_frame < 50 && index / 169 != 7)
                        continue;
                    m_particles.AddSurface(m_surfaces[index], index / 169 % 2
                        ? std::array<float, 4>{ 0.12f, 0.85f, 0.38f, 0.88f }
                        : std::array<float, 4>{ 0.4f, 0.9f, 0.015f, 0.88f }, m_frame / 60.0f);
                }
                m_particles.Update(1.0f / 60.0f);
                m_cpuSum += std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
                const double gpu = GetParticleGpuMilliseconds();
                if (m_frame > 60 && gpu >= 0.0)
                {
                    m_gpuSum += gpu;
                    ++m_gpuSamples;
                }
            }

            void OnPrepareRender() override
            {
                using namespace Engine::GFX;
                UpdateInstanceBuffer(m_instances);
                float position[4];
                float direction[4];
                GetCamera().GetPosition(position);
                GetCamera().GetDirection(direction);
                const auto start = std::chrono::steady_clock::now();
                UpdateParticles(m_particles.PrepareRender({ position[0], position[1], position[2] },
                    { direction[0], direction[1], direction[2] }));
                m_cpuSum += std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
            }

            void OnDraw() override { Engine::GFX::DrawMeshIntances(m_mesh, 1); }
            void OnShutdown() override { m_particles.Clear(); }

            Engine::GFX::cParticleSystem m_particles;
            Engine::GFX::MeshHandle m_mesh = nullptr;
            Engine::GFX::sInstanceData m_instance{};
            std::vector<Engine::GFX::sInstanceData*> m_instances;
            std::vector<Engine::GFX::sParticleSurface> m_surfaces;
            int m_frame = 0;
            int m_gpuSamples = 0;
            double m_gpuSum = 0.0;
            double m_cpuSum = 0.0;
    };
}

int main()
{
    try
    {
        Engine::sAppConfig config{ 1280, 720, c_title, false };
        cParticleRenderCheck application(config);
        application.Run();
    }
    catch (const sFinished&) { return 0; }
    catch (const std::exception& _rError)
    {
        std::cerr << _rError.what() << '\n';
        return 1;
    }
    return 1;
}
