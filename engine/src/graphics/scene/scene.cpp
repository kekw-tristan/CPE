#include "scene.h"

#include <assert.h>

// -------------------------------------------------------------------------------------------------------------------------

namespace Engine::GFX
{

    // -------------------------------------------------------------------------------------------------------------------------

    SceneShapeInstanceHandle cScene::AddShapeInstance(const GFX::sShapeInstance& _rShapeInstance)
    {
        const SceneShapeInstanceHandle handle = static_cast<SceneShapeInstanceHandle>(m_shapeInstances.size());

        m_shapeInstances.push_back(_rShapeInstance);

        return handle;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    SceneShapeInstanceHandle cScene::AddNamedShapeInstance(const std::string& _rName, const GFX::sShapeInstance& _rShapeInstance)
    {
        if (_rName.empty())
            return c_invalidSceneShapeInstanceHandle;

        if (m_namedShapeInstances.find(_rName) != m_namedShapeInstances.end())
            return c_invalidSceneShapeInstanceHandle;

        const SceneShapeInstanceHandle handle = AddShapeInstance(_rShapeInstance);

        m_namedShapeInstances.emplace(_rName, handle);

        return handle;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    GFX::sShapeInstance& cScene::GetShapeInstance(SceneShapeInstanceHandle _shapeInstanceHandle)
    {
        assert(_shapeInstanceHandle < m_shapeInstances.size());
        return m_shapeInstances[_shapeInstanceHandle];
    }

    // -------------------------------------------------------------------------------------------------------------------------

    const GFX::sShapeInstance& cScene::GetShapeInstance(SceneShapeInstanceHandle _shapeInstanceHandle) const
    {
        assert(_shapeInstanceHandle < m_shapeInstances.size());
        return m_shapeInstances[_shapeInstanceHandle];
    }

    // -------------------------------------------------------------------------------------------------------------------------

    SceneShapeInstanceHandle cScene::FindShapeInstanceHandle(const std::string& _rName) const
    {
        const auto iterator = m_namedShapeInstances.find(_rName);

        if (iterator == m_namedShapeInstances.end())
            return c_invalidSceneShapeInstanceHandle;

        return iterator->second;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    const std::vector<GFX::sShapeInstance>& cScene::GetShapeInstances() const
    {
        return m_shapeInstances;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cScene::Clear()
    {
        m_shapeInstances.clear();
        m_namedShapeInstances.clear();
    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------