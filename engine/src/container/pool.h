#pragma once 

#include "allocator.h"

#include <new>

namespace Engine::Container
{
    template<typename T, int tInstancesPerPage>
    class cPool
    {

        public:

            T* Create()
            {
                void* pChunk = m_allocator.AllocateChunk();
                T* pInstance = new (pChunk) T();

                return pInstance; 
            }

            void Destroy(T* _pInstance)
            {
                _pInstance->~T();
                m_allocator.FreeChunk();
            }

        private:

            using cMemoryAllocator = cAllocator<sizeof(T), tInstancesPerPage>;
            cMemoryAllocator m_allocator;

    };
}