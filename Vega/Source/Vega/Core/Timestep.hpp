#pragma once

namespace Vega
{

    class Timestep
    {
    public:
        Timestep(float _Seconds = 0.0f) : m_Seconds(_Seconds) { }

        operator float() const { return m_Seconds; }

        float GetSeconds() const { return m_Seconds; }
        float GetMilliseconds() const { return m_Seconds * 1000.0f; }

    protected:
        float m_Seconds = 0.0f;
    };

}    // namespace Vega
