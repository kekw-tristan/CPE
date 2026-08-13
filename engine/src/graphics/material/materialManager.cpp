#include "materialManager.h"

#include "graphics/material/material.h"

#include <assert.h>
#include <vector>

// -------------------------------------------------------------------------------------------------------------------------

namespace Engine::GFX
{
    
    // -------------------------------------------------------------------------------------------------------------------------

    namespace
    {

        // -------------------------------------------------------------------------------------------------------------------------

        class cMaterialManager
        {
            public:

                static cMaterialManager& GetInstance();

            public:

                MaterialHandle  CreateMaterial(const sMaterial& _rMaterial);
                sMaterial& GetMaterial(MaterialHandle _shapeModelHandle);
                std::vector<sMaterial>& GetMaterials();

            private:

                cMaterialManager();
                ~cMaterialManager();

                cMaterialManager(const cMaterialManager&)       = delete;
                cMaterialManager& operator=(cMaterialManager&)  = delete;

            private:

                std::vector<sMaterial> m_materials;
        };

        // -------------------------------------------------------------------------------------------------------------------------
        
    }

    // -------------------------------------------------------------------------------------------------------------------------

    namespace
    {

        // -------------------------------------------------------------------------------------------------------------------------

        cMaterialManager& cMaterialManager::GetInstance()
        {
            static cMaterialManager s_instance; 
            return s_instance;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        MaterialHandle cMaterialManager::CreateMaterial(const sMaterial& _rMaterial)
        {
            MaterialHandle handle;

            handle = static_cast<MaterialHandle>(m_materials.size());

            m_materials.push_back(_rMaterial);

            return handle;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        sMaterial& cMaterialManager::GetMaterial(MaterialHandle _materialHandle)
        {
            assert(_materialHandle >= 0 && _materialHandle < m_materials.size());
            return m_materials[_materialHandle];
        }

        // -------------------------------------------------------------------------------------------------------------------------

        std::vector<sMaterial>& cMaterialManager::GetMaterials()
        {
            return m_materials; 
        }

        // -------------------------------------------------------------------------------------------------------------------------
        
        cMaterialManager::cMaterialManager()
            : m_materials()
        {
        }

        // -------------------------------------------------------------------------------------------------------------------------

        cMaterialManager::~cMaterialManager()
        {
        }

        // -------------------------------------------------------------------------------------------------------------------------
    }

    // -------------------------------------------------------------------------------------------------------------------------

    namespace MaterialManager
    {

        // -------------------------------------------------------------------------------------------------------------------------
        
        MaterialHandle CreateMaterial(const sMaterial& _rMaterial)
        {
            return cMaterialManager::GetInstance().CreateMaterial(_rMaterial);
        }

        // -------------------------------------------------------------------------------------------------------------------------

        sMaterial& GetMaterial(MaterialHandle _shapeModelHandle)
        {
            return cMaterialManager::GetInstance().GetMaterial(_shapeModelHandle);
        }

        // -------------------------------------------------------------------------------------------------------------------------

        std::vector<sMaterial>& GetMaterials()
        {
            return cMaterialManager::GetInstance().GetMaterials();
        }

        // -------------------------------------------------------------------------------------------------------------------------

    }

    // -------------------------------------------------------------------------------------------------------------------------
   
}

// -------------------------------------------------------------------------------------------------------------------------