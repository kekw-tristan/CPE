#pragma once

#include "graphics/shapeModel/shapeInstance.h"

#include <cstdint>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

namespace Engine::GFX
{
    using SceneShapeInstanceHandle = uint32_t;

    constexpr SceneShapeInstanceHandle c_invalidSceneShapeInstanceHandle = std::numeric_limits<SceneShapeInstanceHandle>::max();

    class cScene
    {
        public:
            SceneShapeInstanceHandle AddShapeInstance(const GFX::sShapeInstance& _rShapeInstance);
            SceneShapeInstanceHandle AddNamedShapeInstance(const std::string& _rName, const GFX::sShapeInstance& _rShapeInstance);

            GFX::sShapeInstance&       GetShapeInstance(SceneShapeInstanceHandle _shapeInstanceHandle);
            const GFX::sShapeInstance& GetShapeInstance(SceneShapeInstanceHandle _shapeInstanceHandle) const;

            SceneShapeInstanceHandle FindShapeInstanceHandle(const std::string& _rName) const;

            const std::vector<GFX::sShapeInstance>& GetShapeInstances() const;

            void Clear();

        private:
            std::vector<GFX::sShapeInstance> m_shapeInstances;
            std::unordered_map<std::string, SceneShapeInstanceHandle> m_namedShapeInstances;
    };
}