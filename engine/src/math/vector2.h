#pragma once

namespace Engine::Math
{
    template<typename T>
    class cVector2
    {

        public:

            constexpr cVector2() = default;

            constexpr cVector2(T _x, T _y)
                : m_x(_x), m_y(_y)
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

        private:

            T m_x = T{};
            T m_y = T{};
    };

    using cVec2f = cVector2<float>;
}
