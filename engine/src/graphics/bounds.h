#pragma once

#include "math/vector3.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <span>

namespace Engine::GFX
{
    using Engine::Math::cVec3f;

    struct sBounds
    {
        cVec3f min{};
        cVec3f max{};
        cVec3f center{};
        cVec3f size{};

        float radius = 0.0f;
    };

    struct sFrustum
    {
        std::array<std::array<float, 4>, 6> planes{};
        std::array<std::array<float, 4>, 6> planeErrors{};

        // Row-vector view * projection, with Vulkan clip depth in [0, w].
        void Set(std::span<const float, 16> _viewProjection)
        {
            for (size_t component = 0; component < 4; ++component)
            {
                const size_t row = component * 4;
                planes[0][component] = _viewProjection[row + 3] + _viewProjection[row];
                planes[1][component] = _viewProjection[row + 3] - _viewProjection[row];
                planes[2][component] = _viewProjection[row + 3] + _viewProjection[row + 1];
                planes[3][component] = _viewProjection[row + 3] - _viewProjection[row + 1];
                planes[4][component] = _viewProjection[row + 2];
                planes[5][component] = _viewProjection[row + 3] - _viewProjection[row + 2];

                // Bound roundoff in the GPU's float clip transform, including cancellation at the far plane.
                constexpr float c_roundoff = 8.0f * std::numeric_limits<float>::epsilon();
                for (size_t plane = 0; plane < planes.size(); ++plane)
                {
                    const size_t axis = plane < 4 ? plane / 2 : 2;
                    planeErrors[plane][component] = c_roundoff * (std::abs(_viewProjection[row + axis])
                        + (plane == 4 ? 0.0f : std::abs(_viewProjection[row + 3])));
                }
            }

            for (size_t index = 0; index < planes.size(); ++index)
            {
                auto& plane = planes[index];
                const float length = std::sqrt(plane[0] * plane[0] + plane[1] * plane[1] + plane[2] * plane[2]);
                if (length > 0.0f)
                {
                    for (size_t component = 0; component < 4; ++component)
                    {
                        plane[component] /= length;
                        planeErrors[index][component] /= length;
                    }
                }
            }
        }

        bool Intersects(const sBounds& _rBounds) const
        {
            constexpr float c_planeTolerance = 0.01f;

            for (size_t index = 0; index < planes.size(); ++index)
            {
                const auto& plane = planes[index];
                const auto& error = planeErrors[index];

                const float x = plane[0] >= 0.0f ? _rBounds.max.x() : _rBounds.min.x();
                const float y = plane[1] >= 0.0f ? _rBounds.max.y() : _rBounds.min.y();
                const float z = plane[2] >= 0.0f ? _rBounds.max.z() : _rBounds.min.z();
                const float tolerance = c_planeTolerance + error[0] * std::max(std::abs(_rBounds.min.x()), std::abs(_rBounds.max.x()))
                    + error[1] * std::max(std::abs(_rBounds.min.y()), std::abs(_rBounds.max.y()))
                    + error[2] * std::max(std::abs(_rBounds.min.z()), std::abs(_rBounds.max.z())) + error[3];

                if (plane[0] * x + plane[1] * y + plane[2] * z + plane[3] < -tolerance)
                    return false;
            }
            return true;
        }
    };
}
