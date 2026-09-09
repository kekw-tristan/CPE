#include "prefabManager.h"

#include <cassert>
#include <vector>

// -------------------------------------------------------------------------------------------------------------------------

namespace Engine::GFX
{

    // -------------------------------------------------------------------------------------------------------------------------

    namespace
    {

        // -------------------------------------------------------------------------------------------------------------------------

        class cPrefabManager
        {

        public:

            static cPrefabManager& GetInstance();


        public:

            PrefabHandle CreatePrefab(const sPrefabDesc& _rPrefabDesc);

            const sPrefabDesc& GetPrefab(PrefabHandle _prefabHandle) const;


        private:

            cPrefabManager();
            ~cPrefabManager();

            cPrefabManager(const cPrefabManager&) = delete;
            cPrefabManager& operator=(const cPrefabManager&) = delete;

            cPrefabManager(cPrefabManager&&) = delete;
            cPrefabManager& operator=(cPrefabManager&&) = delete;


        private:

            std::vector<sPrefabDesc> m_prefabs;

        };

        // -------------------------------------------------------------------------------------------------------------------------

        cPrefabManager& cPrefabManager::GetInstance()
        {
            static cPrefabManager s_instance;

            return s_instance;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        PrefabHandle cPrefabManager::CreatePrefab(const sPrefabDesc& _rPrefabDesc)
        {
            const PrefabHandle prefabHandle = static_cast<PrefabHandle>(m_prefabs.size());

            m_prefabs.push_back(_rPrefabDesc);

            return prefabHandle;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        const sPrefabDesc& cPrefabManager::GetPrefab(PrefabHandle _prefabHandle) const
        {
            assert(_prefabHandle >= 0);
            assert(static_cast<size_t>(_prefabHandle) < m_prefabs.size());

            return m_prefabs[static_cast<size_t>(_prefabHandle)];
        }

        // -------------------------------------------------------------------------------------------------------------------------

        cPrefabManager::cPrefabManager()
            : m_prefabs()
        {
        }

        // -------------------------------------------------------------------------------------------------------------------------

        cPrefabManager::~cPrefabManager()
        {
        }

        // -------------------------------------------------------------------------------------------------------------------------

    }

    // -------------------------------------------------------------------------------------------------------------------------

    namespace PrefabManager
    {

        // -------------------------------------------------------------------------------------------------------------------------

        PrefabHandle CreatePrefab(const sPrefabDesc& _rPrefabDesc)
        {
            return cPrefabManager::GetInstance().CreatePrefab(_rPrefabDesc);
        }

        // -------------------------------------------------------------------------------------------------------------------------

        const sPrefabDesc& GetPrefab(PrefabHandle _prefabHandle)
        {
            return cPrefabManager::GetInstance().GetPrefab(_prefabHandle);
        }

        // -------------------------------------------------------------------------------------------------------------------------

    }

    // -------------------------------------------------------------------------------------------------------------------------

}