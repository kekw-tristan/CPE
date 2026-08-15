#pragma once

#include <array>
#include <cmath>
#include <cstddef>

#include "vector3.h"

namespace Engine::Math
{
    template<typename T>
    class cMatrix4x4
    {

    public:

        constexpr cMatrix4x4() = default;

        constexpr cMatrix4x4(T _m00, T _m01, T _m02, T _m03, T _m10, T _m11, T _m12, T _m13, T _m20, T _m21, T _m22, T _m23, T _m30, T _m31, T _m32, T _m33) noexcept
            : m_values
            {
                _m00, _m01, _m02, _m03,
                _m10, _m11, _m12, _m13,
                _m20, _m21, _m22, _m23,
                _m30, _m31, _m32, _m33
            }
        {
        }

        constexpr explicit cMatrix4x4(const std::array<T, 16>& _rValues) noexcept
            : m_values(_rValues)
        {
        }

        constexpr T& operator()(std::size_t _row, std::size_t _column) noexcept
        {
            return m_values[_row * 4 + _column];
        }

        constexpr const T& operator()(std::size_t _row, std::size_t _column) const noexcept
        {
            return m_values[_row * 4 + _column];
        }

        constexpr T* data() noexcept
        {
            return m_values.data();
        }

        constexpr const T* data() const noexcept
        {
            return m_values.data();
        }

        static constexpr std::size_t elementCount() noexcept
        {
            return 16;
        }

        constexpr cMatrix4x4 operator+(const cMatrix4x4& _rOther) const noexcept
        {
            cMatrix4x4 result = zero();

            for (std::size_t index = 0; index < elementCount(); ++index)
            {
                result.m_values[index] = m_values[index] + _rOther.m_values[index];
            }

            return result;
        }

        constexpr cMatrix4x4 operator-(const cMatrix4x4& _rOther) const noexcept
        {
            cMatrix4x4 result = zero();

            for (std::size_t index = 0; index < elementCount(); ++index)
            {
                result.m_values[index] = m_values[index] - _rOther.m_values[index];
            }

            return result;
        }

        constexpr cMatrix4x4 operator-() const noexcept
        {
            cMatrix4x4 result = zero();

            for (std::size_t index = 0; index < elementCount(); ++index)
            {
                result.m_values[index] = -m_values[index];
            }

            return result;
        }

        constexpr cMatrix4x4 operator*(T _scalar) const noexcept
        {
            cMatrix4x4 result = zero();

            for (std::size_t index = 0; index < elementCount(); ++index)
            {
                result.m_values[index] = m_values[index] * _scalar;
            }

            return result;
        }

        constexpr cMatrix4x4 operator/(T _scalar) const noexcept
        {
            cMatrix4x4 result = zero();

            for (std::size_t index = 0; index < elementCount(); ++index)
            {
                result.m_values[index] = m_values[index] / _scalar;
            }

            return result;
        }

        constexpr cMatrix4x4 operator*(const cMatrix4x4& _rOther) const noexcept
        {
            cMatrix4x4 result = zero();

            for (std::size_t row = 0; row < 4; ++row)
            {
                for (std::size_t column = 0; column < 4; ++column)
                {
                    for (std::size_t index = 0; index < 4; ++index)
                    {
                        result(row, column) += (*this)(row, index) * _rOther(index, column);
                    }
                }
            }

            return result;
        }

        constexpr cMatrix4x4& operator+=(const cMatrix4x4& _rOther) noexcept
        {
            for (std::size_t index = 0; index < elementCount(); ++index)
            {
                m_values[index] += _rOther.m_values[index];
            }

            return *this;
        }

        constexpr cMatrix4x4& operator-=(const cMatrix4x4& _rOther) noexcept
        {
            for (std::size_t index = 0; index < elementCount(); ++index)
            {
                m_values[index] -= _rOther.m_values[index];
            }

            return *this;
        }

        constexpr cMatrix4x4& operator*=(T _scalar) noexcept
        {
            for (T& rValue : m_values)
            {
                rValue *= _scalar;
            }

            return *this;
        }

        constexpr cMatrix4x4& operator/=(T _scalar) noexcept
        {
            for (T& rValue : m_values)
            {
                rValue /= _scalar;
            }

            return *this;
        }

        constexpr cMatrix4x4& operator*=(const cMatrix4x4& _rOther) noexcept
        {
            *this = *this * _rOther;

            return *this;
        }

        constexpr bool operator==(const cMatrix4x4& _rOther) const noexcept
        {
            for (std::size_t index = 0; index < elementCount(); ++index)
            {
                if (m_values[index] != _rOther.m_values[index])
                {
                    return false;
                }
            }

            return true;
        }

        constexpr bool operator!=(const cMatrix4x4& _rOther) const noexcept
        {
            return !(*this == _rOther);
        }

        constexpr cMatrix4x4 transposed() const noexcept
        {
            return
            {
                m_values[0], m_values[4], m_values[8], m_values[12],
                m_values[1], m_values[5], m_values[9], m_values[13],
                m_values[2], m_values[6], m_values[10], m_values[14],
                m_values[3], m_values[7], m_values[11], m_values[15]
            };
        }

        constexpr void transpose() noexcept
        {
            *this = transposed();
        }

        constexpr cVector3<T> transformPoint(const cVector3<T>& _rPoint) const noexcept
        {
            return
            {
                _rPoint.x() * m_values[0] + _rPoint.y() * m_values[4] + _rPoint.z() * m_values[8] + m_values[12],
                _rPoint.x() * m_values[1] + _rPoint.y() * m_values[5] + _rPoint.z() * m_values[9] + m_values[13],
                _rPoint.x() * m_values[2] + _rPoint.y() * m_values[6] + _rPoint.z() * m_values[10] + m_values[14]
            };
        }

        constexpr cVector3<T> transformDirection(const cVector3<T>& _rDirection) const noexcept
        {
            return
            {
                _rDirection.x() * m_values[0] + _rDirection.y() * m_values[4] + _rDirection.z() * m_values[8],
                _rDirection.x() * m_values[1] + _rDirection.y() * m_values[5] + _rDirection.z() * m_values[9],
                _rDirection.x() * m_values[2] + _rDirection.y() * m_values[6] + _rDirection.z() * m_values[10]
            };
        }

        constexpr cVector3<T> translation() const noexcept
        {
            return
            {
                m_values[12],
                m_values[13],
                m_values[14]
            };
        }

        constexpr void setTranslation(const cVector3<T>& _rTranslation) noexcept
        {
            m_values[12] = _rTranslation.x();
            m_values[13] = _rTranslation.y();
            m_values[14] = _rTranslation.z();
        }

        static constexpr cMatrix4x4 identity() noexcept
        {
            return {};
        }

        static constexpr cMatrix4x4 zero() noexcept
        {
            return
            {
                T{}, T{}, T{}, T{},
                T{}, T{}, T{}, T{},
                T{}, T{}, T{}, T{},
                T{}, T{}, T{}, T{}
            };
        }

        static constexpr cMatrix4x4 translation(const cVector3<T>& _rTranslation) noexcept
        {
            return
            {
                T{1}, T{}, T{}, T{},
                T{}, T{1}, T{}, T{},
                T{}, T{}, T{1}, T{},
                _rTranslation.x(), _rTranslation.y(), _rTranslation.z(), T{1}
            };
        }

        static constexpr cMatrix4x4 scale(const cVector3<T>& _rScale) noexcept
        {
            return
            {
                _rScale.x(), T{}, T{}, T{},
                T{}, _rScale.y(), T{}, T{},
                T{}, T{}, _rScale.z(), T{},
                T{}, T{}, T{}, T{1}
            };
        }

        static constexpr cMatrix4x4 scale(T _scalar) noexcept
        {
            return scale({ _scalar, _scalar, _scalar });
        }

        static cMatrix4x4 rotationX(T _radians) noexcept
        {
            const T cosine = std::cos(_radians);
            const T sine = std::sin(_radians);

            return
            {
                T{1}, T{}, T{}, T{},
                T{}, cosine, sine, T{},
                T{}, -sine, cosine, T{},
                T{}, T{}, T{}, T{1}
            };
        }

        static cMatrix4x4 rotationY(T _radians) noexcept
        {
            const T cosine = std::cos(_radians);
            const T sine = std::sin(_radians);

            return
            {
                cosine, T{}, -sine, T{},
                T{}, T{1}, T{}, T{},
                sine, T{}, cosine, T{},
                T{}, T{}, T{}, T{1}
            };
        }

        static cMatrix4x4 rotationZ(T _radians) noexcept
        {
            const T cosine = std::cos(_radians);
            const T sine = std::sin(_radians);

            return
            {
                cosine, sine, T{}, T{},
                -sine, cosine, T{}, T{},
                T{}, T{}, T{1}, T{},
                T{}, T{}, T{}, T{1}
            };
        }

        static cMatrix4x4 lookAtRH(const cVector3<T>& _rEye, const cVector3<T>& _rTarget, const cVector3<T>& _rUp) noexcept
        {
            auto normalize = [](const cVector3<T>& _rVector)
                {
                    const T length = std::sqrt(_rVector.x() * _rVector.x() + _rVector.y() * _rVector.y() + _rVector.z() * _rVector.z());

                    if (length <= T{ 0 })
                    {
                        return cVector3<T>{ T{}, T{}, T{} };
                    }

                    return cVector3<T>{ _rVector.x() / length, _rVector.y() / length, _rVector.z() / length };
                };

            auto dot = [](const cVector3<T>& _rA, const cVector3<T>& _rB)
                {
                    return _rA.x() * _rB.x() + _rA.y() * _rB.y() + _rA.z() * _rB.z();
                };

            auto cross = [](const cVector3<T>& _rA, const cVector3<T>& _rB)
                {
                    return cVector3<T>
                    {
                        _rA.y()* _rB.z() - _rA.z() * _rB.y(),
                            _rA.z()* _rB.x() - _rA.x() * _rB.z(),
                            _rA.x()* _rB.y() - _rA.y() * _rB.x()
                    };
                };

            const cVector3<T> zAxis = normalize(
                {
                    _rEye.x() - _rTarget.x(),
                    _rEye.y() - _rTarget.y(),
                    _rEye.z() - _rTarget.z()
                });

            const cVector3<T> xAxis = normalize(cross(_rUp, zAxis));
            const cVector3<T> yAxis = cross(zAxis, xAxis);

            return
            {
                xAxis.x(),                  yAxis.x(),                  zAxis.x(),                  T{},
                xAxis.y(),                  yAxis.y(),                  zAxis.y(),                  T{},
                xAxis.z(),                  yAxis.z(),                  zAxis.z(),                  T{},
                -dot(xAxis, _rEye),         -dot(yAxis, _rEye),         -dot(zAxis, _rEye),         T{1}
            };
        }

        static cMatrix4x4 orthographicRH(T _left, T _right, T _bottom, T _top, T _nearPlane, T _farPlane) noexcept
        {
            return
            {
                T{2} / (_right - _left),     T{},                     T{},                                  T{},
                T{},                         T{2} / (_top - _bottom), T{},                                  T{},
                T{},                         T{},                     T{1} / (_nearPlane - _farPlane),      T{},
                
                -(_right + _left) / (_right - _left),
                -(_top + _bottom) / (_top - _bottom),
                _nearPlane / (_nearPlane - _farPlane),
                T{1}
            };
        }

        static cMatrix4x4 perspectiveRH(T _fieldOfView, T _aspectRatio, T _nearPlane, T _farPlane) noexcept
        {
            const T yScale = T{ 1 } / std::tan(_fieldOfView * T{ 0.5 });
            const T xScale = yScale / _aspectRatio;

            return
            {
                xScale, T{},    T{},                                                 T{},
                T{},    yScale, T{},                                                 T{},
                T{},    T{},    _farPlane / (_nearPlane - _farPlane),                T{-1},
                T{},    T{},    (_nearPlane * _farPlane) / (_nearPlane - _farPlane), T{}
            };
        }

        friend constexpr cMatrix4x4 operator*(T _scalar, const cMatrix4x4& _rMatrix) noexcept
        {
            return _rMatrix * _scalar;
        }

    private:

        std::array<T, 16> m_values
        {
            T{1}, T{}, T{}, T{},
            T{}, T{1}, T{}, T{},
            T{}, T{}, T{1}, T{},
            T{}, T{}, T{}, T{1}
        };
    };

    using cMatrix4x4f = cMatrix4x4<float>;
    using cMatrix4x4d = cMatrix4x4<double>;
}