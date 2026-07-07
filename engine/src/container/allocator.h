#pragma once

#include <assert.h>
#include <cstdint>
#include <iostream>
#include <stdlib.h>

namespace Engine::Container
{

    template<int tChunkByteSize, int tChunksPerPage>
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
                    free(pPage);
                }
            }

        public: 

            void* AllocateChunk()
            {
                if (!HasFreeSlot())
                {
                    sPage* pPage = static_cast<sPage*>(malloc(sizeof(sPage)));

                    PushPage(pPage);

                    std::cout << "Pushed new Pool Page\n";

                    for (int index = tChunksPerPage - 1; index >= 0; --index)
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

            struct sChunk
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