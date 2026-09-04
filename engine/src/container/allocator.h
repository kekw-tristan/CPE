#pragma once

#include <assert.h>
#include <cstdint>
#include <cstddef>
#include <iostream>
#include <new>

namespace Engine::Container
{

    template<std::size_t tChunkByteSize, std::size_t tChunksPerPage, std::size_t tAlignment = alignof(std::max_align_t)>
    class cAllocator
    {

        public: 

            cAllocator()
                : m_pFirstPage(nullptr)
                , m_pFirstFreeChunk(nullptr)
            {
            }

           ~cAllocator()
            {
                while(m_pFirstPage != nullptr)
                {
                    sPage* pPage = PopPage(); 
                    ::operator delete(pPage, std::align_val_t{ alignof(sPage) });
                }
            }

        public: 

            void* AllocateChunk()
            {
                if (!HasFreeSlot())
                {
                    sPage* pPage = static_cast<sPage*>(::operator new(sizeof(sPage), std::align_val_t{ alignof(sPage) }));

                    PushPage(pPage);

                    std::cout << "Pushed new Pool Page\n";

                    for (std::size_t index = tChunksPerPage; index-- > 0;)
                    {
                        sChunk* pChunk = &pPage->chunks[index];
                        PushFreeChunk(pChunk);
                    }
                }

                sChunk* pFreeSlot = PopFreeChunk();
                return pFreeSlot;
            }

            void FreeChunk(void* _pChunk)
            {
                sChunk* pChunk = static_cast<sChunk*>(_pChunk);
                PushFreeChunk(pChunk);
            }

        private: 

            static constexpr std::size_t c_chunkAlignment = tAlignment > alignof(void*) ? tAlignment : alignof(void*);

            struct alignas(c_chunkAlignment) sChunk
            {
                union 
                {
                    uint8_t bytes[tChunkByteSize];
                    sChunk* pNextFreeChunk; 
                };   
            };

            struct sPage
            {
                sChunk chunks[tChunksPerPage]; 
                sPage* pNextPage;
            };

        private:

            sPage*  m_pFirstPage; 
            sChunk* m_pFirstFreeChunk;

        private:

            void PushFreeChunk(sChunk* _pChunk)
            {
                _pChunk->pNextFreeChunk = m_pFirstFreeChunk;
                m_pFirstFreeChunk = _pChunk;
            }

            sChunk* PopFreeChunk()
            {
                assert(m_pFirstFreeChunk != nullptr);

                sChunk* pChunk      = m_pFirstFreeChunk;
                m_pFirstFreeChunk   = m_pFirstFreeChunk->pNextFreeChunk;
                return pChunk;
            }

            bool HasFreeSlot()
            {
                return m_pFirstFreeChunk != nullptr;
            }

            void PushPage(sPage* _pPage)
            {
                _pPage->pNextPage = m_pFirstPage;
                m_pFirstPage = _pPage;
            }

            sPage* PopPage()
            {
                assert(m_pFirstPage != nullptr);

                sPage* pPage = m_pFirstPage; 
                m_pFirstPage = m_pFirstPage->pNextPage; 
                return pPage;
            }
    };
}
