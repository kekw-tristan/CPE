#pragma once

#include "graphics/shapeModel/prefabManager.h"
#include "graphics/transform.h"


namespace Engine::GFX
{
    class cScene;
}


namespace World
{

    void InstantiatePrefab(
        Engine::GFX::cScene& _rScene,
        Engine::GFX::PrefabHandle _prefabHandle,
        const Engine::GFX::sTransform& _rTransform
    );

}
