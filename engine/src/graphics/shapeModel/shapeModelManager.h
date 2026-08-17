#pragma once

namespace Engine::GFX
{
    using ShapeModelHandle = int;

    struct sShapeModelDesc;

    namespace ShapeModelManager
    {
        ShapeModelHandle CreateShapeModel(const sShapeModelDesc& _rDesc); 
        const sShapeModelDesc& GetShapeModel(ShapeModelHandle _shapeModelHandle);
        void UpdateShapeModel(ShapeModelHandle _shapeModelHandle, const sShapeModelDesc& _rDesc);
    }

}