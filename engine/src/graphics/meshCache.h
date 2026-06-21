#pragma once 

#include "graphics/meshData.h"
#include "graphics/meshGenerator.h"

#include <unordered_map>
#include <memory>
#include <string>

namespace Engine::GFX
{
    class cMeshCache
    {
        public:

            const sMeshData& GetCube(const sCubeDesc& _rDesc);

        private:

            static std::string MakeKey(const sCubeDesc& _rDesc);

        private:

            std::unordered_map<std::string, sMeshData> m_cache; 
    };
}