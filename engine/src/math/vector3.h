#pragma once

#include <cmath>

namespace Engine::Math
{
    template<typename T>
    class cVector3
    {

        public:

            constexpr cVector3() = default;

            constexpr cVector3(T _x, T _y, T _z)
                : m_x(_x), m_y(_y), m_z(_z)
            {
            }

            constexpr T x() const noexcept
            {
                return m_x;
            }

            constexpr T y() const noexcept
            {
                return m_y;
            }

            constexpr T z() const noexcept
            {
                return m_z;
            }

            constexpr cVector3 operator+(const cVector3& _rOther) const noexcept
            {
                return
                {
                    m_x + _rOther.m_x,
                    m_y + _rOther.m_y,
                    m_z + _rOther.m_z
                };
            }

            constexpr cVector3 operator-(const cVector3& _rOther) const noexcept
            {
                return
                {
                    m_x - _rOther.m_x,
                    m_y - _rOther.m_y,
                    m_z - _rOther.m_z
                };
            }

            constexpr cVector3 operator-() const noexcept
            {
                return
                {
                    -m_x,
                    -m_y,
                    -m_z
                };
            }

            constexpr cVector3 operator*(T _scalar) const noexcept
            {
                return
                {
                    m_x * _scalar,
                    m_y * _scalar,
                    m_z * _scalar
                };
            }

            constexpr cVector3 operator/(T _scalar) const noexcept
            {
                return
                {
                    m_x / _scalar,
                    m_y / _scalar,
                    m_z / _scalar
                };
            }

            constexpr cVector3& operator+=(const cVector3& _rOther) noexcept
            {
                m_x += _rOther.m_x;
                m_y += _rOther.m_y;
                m_z += _rOther.m_z;

                return *this;
            }

            constexpr cVector3& operator-=(const cVector3& _rOther) noexcept
            {
                m_x -= _rOther.m_x;
                m_y -= _rOther.m_y;
                m_z -= _rOther.m_z;

                return *this;
            }

            constexpr cVector3& operator*=(T _scalar) noexcept
            {
                m_x *= _scalar;
                m_y *= _scalar;
                m_z *= _scalar;

                return *this;
            }

            constexpr cVector3& operator/=(T _scalar) noexcept
            {
                m_x /= _scalar;
                m_y /= _scalar;
                m_z /= _scalar;

                return *this;
            }

            constexpr bool operator==(const cVector3& _rOther) const noexcept
            {
                return m_x == _rOther.m_x
                    && m_y == _rOther.m_y
                    && m_z == _rOther.m_z;
            }

            constexpr bool operator!=(const cVector3& _rOther) const noexcept
            {
                return !(*this == _rOther);
            }

            constexpr T dot(const cVector3& _rOther) const noexcept
            {
                return m_x * _rOther.m_x
                    + m_y * _rOther.m_y
                    + m_z * _rOther.m_z;
            }

            constexpr cVector3 cross(const cVector3& _rOther) const noexcept
            {
                return
                {
                    m_y * _rOther.m_z - m_z * _rOther.m_y,
                    m_z * _rOther.m_x - m_x * _rOther.m_z,
                    m_x * _rOther.m_y - m_y * _rOther.m_x
                };
            }

            constexpr T lengthSquared() const noexcept
            {
                return dot(*this);
            }

            T length() const noexcept
            {
                return std::sqrt(lengthSquared());
            }

            cVector3 normalized() const noexcept
            {
                const T _length = length();

                if (_length == T{})
                {
                    return {};
                }

                return *this / _length;
            }

            void normalize() noexcept
            {
                const T _length = length();

                if (_length != T{})
                {
                    *this /= _length;
                }
            }

            constexpr bool isZero() const noexcept
            {
                return m_x == T{}
                    && m_y == T{}
                && m_z == T{};
            }

            static constexpr T distanceSquared(const cVector3& _rFirst, const cVector3& _rSecond) noexcept
            {
                return (_rSecond - _rFirst).lengthSquared();
            }

            static T distance(const cVector3& _rFirst, const cVector3& _rSecond) noexcept
            {
                return (_rSecond - _rFirst).length();
            }

            static constexpr cVector3 lerp(const cVector3& _rStart, const cVector3& _rEnd, T _factor) noexcept
            {
                return _rStart + (_rEnd - _rStart) * _factor;
            }

            friend constexpr cVector3 operator*(T _scalar, const cVector3& _rVector) noexcept
            {
                return _rVector * _scalar;
            }

        private:

            T m_x{};
            T m_y{};
            T m_z{};
    };

    using cVec3f = cVector3<float>;
    using cVec3d = cVector3<double>;
}