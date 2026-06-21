#include "meshCache.h"

#include <sstream>

// -------------------------------------------------------------------------------------------------------------------------

namespace Engine::GFX
{

    // -------------------------------------------------------------------------------------------------------------------------

    const sMeshData &cMeshCache::GetCube(const sCubeDesc &_rDesc)
    {
        const std::string key = MakeKey(_rDesc);

        auto iterator = m_cache.find(key); 

        if (iterator != m_cache.end())
        {
            return iterator->second;
        }

        sMeshData mesh = cMeshGenerator::CreateCube(_rDesc);

        auto [insertedIterator, _] = m_cache.emplace(key, std::move(mesh));

        return insertedIterator->second;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    std::string cMeshCache::MakeKey(const sCubeDesc &_rDesc)
    {
        std::ostringstream stream;

        stream << "Cube_"
               << _rDesc.width      << "_"
               << _rDesc.height     << "_"
               << _rDesc.depth      << "_"
               << _rDesc.color[0]   << "_"
               << _rDesc.color[1]   << "_"
               << _rDesc.color[2]   << "_"
               << _rDesc.color[3]   << "_";

        return stream.str();
    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------