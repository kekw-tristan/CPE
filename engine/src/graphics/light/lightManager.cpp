#include "lightManager.h"

#include "graphics/gfxConfig.h"
#include "graphics/light/light.h"

#include <assert.h>
#include <unordered_map>
#include <vector>

// -------------------------------------------------------------------------------------------------------------------------

namespace Engine::GFX
{

    // -------------------------------------------------------------------------------------------------------------------------

    namespace
    {

        // -------------------------------------------------------------------------------------------------------------------------

        class cLightManager
        {
            public:

                static cLightManager& GetInstance();

            public:

                LightHandle                CreateLight(const sLight& _rLight);
                bool                       DestroyLight(LightHandle _lightHandle);
                bool                       UpdateLight(LightHandle _lightHandle, const sLight& _rLight);
                sLight*                    TryGetLight(LightHandle _lightHandle);
                sLight&                    GetLight(LightHandle _lightHandle);
                const std::vector<sLight>& GetLights();

            private:

                cLightManager();
               ~cLightManager();

                cLightManager(const cLightManager&)         = delete;
                cLightManager& operator=(cLightManager&)    = delete;

            private:

                std::vector<sLight>                     m_lights;
                std::vector<LightHandle>                m_lightHandles;
                std::unordered_map<LightHandle, size_t> m_lightIndices;
                LightHandle                            m_nextLightHandle;
        };

        // -------------------------------------------------------------------------------------------------------------------------

    }

    // -------------------------------------------------------------------------------------------------------------------------

    namespace
    {

        // -------------------------------------------------------------------------------------------------------------------------

        cLightManager& cLightManager::GetInstance()
        {
            static cLightManager s_instance;
            return s_instance;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        LightHandle cLightManager::CreateLight(const sLight& _rLight)
        {
            if (m_lights.size() >= c_maxNumberOfLights)
                return c_invalidLightHandle;

            const LightHandle handle = m_nextLightHandle++;

            m_lightIndices.emplace(handle, m_lights.size());
            m_lights.push_back(_rLight);
            m_lightHandles.push_back(handle);

            return handle;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        bool cLightManager::DestroyLight(LightHandle _lightHandle)
        {
            const auto iterator = m_lightIndices.find(_lightHandle);

            if (iterator == m_lightIndices.end())
                return false;

            const size_t lightIndex = iterator->second;
            const size_t lastIndex  = m_lights.size() - 1;

            if (lightIndex != lastIndex)
            {
                m_lights[lightIndex]       = std::move(m_lights[lastIndex]);
                m_lightHandles[lightIndex] = m_lightHandles[lastIndex];
                m_lightIndices[m_lightHandles[lightIndex]] = lightIndex;
            }

            m_lights.pop_back();
            m_lightHandles.pop_back();
            m_lightIndices.erase(iterator);

            return true;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        bool cLightManager::UpdateLight(LightHandle _lightHandle, const sLight& _rLight)
        {
            sLight* pLight = TryGetLight(_lightHandle);

            if (pLight == nullptr)
                return false;

            *pLight = _rLight;
            return true;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        sLight* cLightManager::TryGetLight(LightHandle _lightHandle)
        {
            const auto iterator = m_lightIndices.find(_lightHandle);

            if (iterator == m_lightIndices.end())
                return nullptr;

            return &m_lights[iterator->second];
        }

        // -------------------------------------------------------------------------------------------------------------------------

        sLight& cLightManager::GetLight(LightHandle _lightHandle)
        {
            sLight* pLight = TryGetLight(_lightHandle);

            assert(pLight != nullptr);
            return *pLight;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        const std::vector<sLight>& cLightManager::GetLights()
        {
            return m_lights;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        cLightManager::cLightManager()
            : m_lights()
            , m_lightHandles()
            , m_lightIndices()
            , m_nextLightHandle(0)
        {
        }

        // -------------------------------------------------------------------------------------------------------------------------

        cLightManager::~cLightManager()
        {
        }

        // -------------------------------------------------------------------------------------------------------------------------
    }

    // -------------------------------------------------------------------------------------------------------------------------

    namespace LightManager
    {

        // -------------------------------------------------------------------------------------------------------------------------

        LightHandle CreateLight(const sLight& _rLight)
        {
            return cLightManager::GetInstance().CreateLight(_rLight);
        }

        // -------------------------------------------------------------------------------------------------------------------------

        bool DestroyLight(LightHandle _lightHandle)
        {
            return cLightManager::GetInstance().DestroyLight(_lightHandle);
        }

        // -------------------------------------------------------------------------------------------------------------------------

        bool UpdateLight(LightHandle _lightHandle, const sLight& _rLight)
        {
            return cLightManager::GetInstance().UpdateLight(_lightHandle, _rLight);
        }

        // -------------------------------------------------------------------------------------------------------------------------

        sLight* TryGetLight(LightHandle _lightHandle)
        {
            return cLightManager::GetInstance().TryGetLight(_lightHandle);
        }

        // -------------------------------------------------------------------------------------------------------------------------

        sLight& GetLight(LightHandle _lightHandle)
        {
            return cLightManager::GetInstance().GetLight(_lightHandle);
        }

        // -------------------------------------------------------------------------------------------------------------------------

        const std::vector<sLight>& GetLights()
        {
            return cLightManager::GetInstance().GetLights(); 
        }

        // -------------------------------------------------------------------------------------------------------------------------

    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------
