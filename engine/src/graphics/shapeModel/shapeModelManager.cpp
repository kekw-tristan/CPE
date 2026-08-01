#include "shapeModelManager.h"

#include "graphics/shapeModel/shapeModelDesc.h"

#include <assert.h>
#include <vector>

// -------------------------------------------------------------------------------------------------------------------------

namespace Engine::GFX
{

    // -------------------------------------------------------------------------------------------------------------------------

    namespace
    {
        class cShapeModelManager
        {

            public:

                static cShapeModelManager& GetInstance();

            public:

                ShapeModelHandle CreateShapeModel(const sShapeModelDesc& _rDesc); 
                sShapeModelDesc& GetShapeModel(ShapeModelHandle _shapeModelHandle);

            private:

                cShapeModelManager();
               ~cShapeModelManager();

               cShapeModelManager(const cShapeModelManager&)        = delete;
               cShapeModelManager& operator=(cShapeModelManager&)   = delete; 
               
            private:

                std::vector<sShapeModelDesc> m_shapeModels;
        };
    }

    // -------------------------------------------------------------------------------------------------------------------------

    namespace 
    {

        // -------------------------------------------------------------------------------------------------------------------------

        cShapeModelManager::cShapeModelManager()
            : m_shapeModels()
        {
        }

        // -------------------------------------------------------------------------------------------------------------------------

        cShapeModelManager::~cShapeModelManager()
        {

        }

        // -------------------------------------------------------------------------------------------------------------------------

        cShapeModelManager& cShapeModelManager::GetInstance()
        {
            static cShapeModelManager s_instance;
            return s_instance;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        ShapeModelHandle cShapeModelManager::CreateShapeModel(const sShapeModelDesc& _rDesc)
        {
            ShapeModelHandle handle;

            handle = static_cast<ShapeModelHandle>(m_shapeModels.size());

            m_shapeModels.push_back(_rDesc);

            return handle;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        sShapeModelDesc& cShapeModelManager::GetShapeModel(ShapeModelHandle _shapeModelHandle)
        {
            assert(_shapeModelHandle >= 0 && _shapeModelHandle < m_shapeModels.size());
            return m_shapeModels[_shapeModelHandle];
        }

        // -------------------------------------------------------------------------------------------------------------------------

    }

    // -------------------------------------------------------------------------------------------------------------------------

    namespace ShapeModelManager
    {

        // -------------------------------------------------------------------------------------------------------------------------

        ShapeModelHandle CreateShapeModel(const sShapeModelDesc& _rDesc)
        {
            return cShapeModelManager::GetInstance().CreateShapeModel(_rDesc); 
        }

        // -------------------------------------------------------------------------------------------------------------------------

        sShapeModelDesc& GetShapeModel(ShapeModelHandle _shapeModelHandle)
        {
            return cShapeModelManager::GetInstance().GetShapeModel(_shapeModelHandle);
        }

        // -------------------------------------------------------------------------------------------------------------------------

    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------