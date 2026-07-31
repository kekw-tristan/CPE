#pragma once

namespace Engine::GFX
{
    using ShapeModelHandle = int;

    struct sShapeModelDesc;

    namespace ShapeModelManager
    {
        ShapeModelHandle CreateShapeModel(const sShapeModelDesc& _rDesc); 
        sShapeModelDesc& GetShapeModel(ShapeModelHandle _shapeModelHandle);
    }

}