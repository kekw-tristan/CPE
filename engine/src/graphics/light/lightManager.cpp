#include "lightManager.h"

#include "graphics/light/light.h"

#include <assert.h>
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

                LightHandle             CreateLight(const sLight& _rLight);
                sLight&                 GetLight(LightHandle _lightHandle);
                std::vector<sLight>&    GetLights();

            private:

                cLightManager();
               ~cLightManager();

                cLightManager(const cLightManager&)         = delete;
                cLightManager& operator=(cLightManager&)    = delete;

            private:

                std::vector<sLight> m_lights;
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
            LightHandle handle;

            handle = static_cast<LightHandle>(m_lights.size());

            m_lights.push_back(_rLight);

            return handle;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        sLight& cLightManager::GetLight(LightHandle _lightHandle)
        {
            assert(_lightHandle >= 0 && _lightHandle < m_lights.size());
            return m_lights[_lightHandle];
        }

        // -------------------------------------------------------------------------------------------------------------------------

        std::vector<sLight>& cLightManager::GetLights()
        {
            return m_lights;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        cLightManager::cLightManager()
            : m_lights()
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

        sLight& GetLight(LightHandle _lightHandle)
        {
            return cLightManager::GetInstance().GetLight(_lightHandle);
        }

        // -------------------------------------------------------------------------------------------------------------------------

        std::vector<sLight>& GetLights()
        {
            return cLightManager::GetInstance().GetLights(); 
        }

        // -------------------------------------------------------------------------------------------------------------------------

    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------