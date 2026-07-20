#pragma once 

#include "container/pool.h"

namespace Engine::Container
{
    class cManagedObject
    {
        
        public:

            static void IncReference(cManagedObject* _pObject)
            {
                ++ _pObject->m_numberOfReferences;
            }

            static void DecReference(cManagedObject* _pObject)
            {
                -- _pObject->m_numberOfReferences;

                if (_pObject->m_numberOfReferences == 0)
                {
                    _pObject->m_destroyFtr(_pObject->m_pManagedPool, _pObject);
                }
            }

        private:

            using FDestroy = void (*) (void*, cManagedObject*); 

            int         m_numberOfReferences;
            void*       m_pManagedPool;
            FDestroy    m_destroyFtr;

        private:

            template <typename T, int tNumberOfInstancesPerPage> 
            friend class cManagedPool;
    };
}

