#pragma once

#include "Vega/Core/Base.hpp"

#include "Vega/Utils/Logger.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/string_cast.hpp"

namespace Vega
{

    class Log
    {
    public:
        static void Init();

        static Ref<Logger> GetCoreLogger() { return s_CoreLogger; }
        static Ref<Logger> GetClientLogger() { return s_ClientLogger; }

        static void SetPluginCoreLogger(Ref<Logger> _Logger) { s_CoreLogger = _Logger; }
        static void SetPluginClientLogger(Ref<Logger> _Logger) { s_ClientLogger = _Logger; }

    protected:
        static inline Ref<Logger> s_CoreLogger;
        static inline Ref<Logger> s_ClientLogger;
    };

}    // namespace Vega

template <typename OStream, glm::length_t L, typename T, glm::qualifier Q>
inline OStream& operator<<(OStream& os, const glm::vec<L, T, Q>& vector)
{
    return os << glm::to_string(vector);
}

template <typename OStream, glm::length_t C, glm::length_t R, typename T, glm::qualifier Q>
inline OStream& operator<<(OStream& os, const glm::mat<C, R, T, Q>& matrix)
{
    return os << glm::to_string(matrix);
}

template <typename OStream, typename T, glm::qualifier Q>
inline OStream& operator<<(OStream& os, glm::qua<T, Q> quaternion)
{
    return os << glm::to_string(quaternion);
}

// Core log macros
#define VEGA_CORE_TRACE(...)    ::Vega::Log::GetCoreLogger()->Trace(__VA_ARGS__)
#define VEGA_CORE_INFO(...)     ::Vega::Log::GetCoreLogger()->Info(__VA_ARGS__)
#define VEGA_CORE_WARN(...)     ::Vega::Log::GetCoreLogger()->Warn(__VA_ARGS__)
#define VEGA_CORE_ERROR(...)    ::Vega::Log::GetCoreLogger()->Error(__VA_ARGS__)
#define VEGA_CORE_CRITICAL(...) ::Vega::Log::GetCoreLogger()->Critical(__VA_ARGS__)

// Client log macros
#define VEGA_TRACE(...)    ::Vega::Log::GetClientLogger()->Trace(__VA_ARGS__)
#define VEGA_INFO(...)     ::Vega::Log::GetClientLogger()->Info(__VA_ARGS__)
#define VEGA_WARN(...)     ::Vega::Log::GetClientLogger()->Warn(__VA_ARGS__)
#define VEGA_ERROR(...)    ::Vega::Log::GetClientLogger()->Error(__VA_ARGS__)
#define VEGA_CRITICAL(...) ::Vega::Log::GetClientLogger()->Critical(__VA_ARGS__)
