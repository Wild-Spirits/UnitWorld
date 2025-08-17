
#include "Vega/Core/Base.hpp"

#include "Vega/Utils/utf8.hpp"

#include <iostream>
#include <mutex>
#include <utility>

namespace Vega
{

    // TODO: add selection of stream handle for color change by enum (log/error).
    // TODO: like cout or cerr. Default cout?
    class Logger
    {
    public:
#ifdef _WIN32
        enum class ConsoleTxtColor : uint32_t
        {
            Black = 30,    // +
            Blue = 34,     // +
            Green = 32,    // +
            // Cyan = 3,
            Red = 31,        // +
            Magenta = 35,    // +
            // Brown        = 6, // No bash Color!
            // LightGray = 7,
            // DarkGray = 8,
            LightBlue = 36,    // +
            // LightGreen = 10,
            // LightCyan = 11,
            // LightRed = 12,
            // LightMagenta = 13,
            Yellow = 33,    // +
            White = 37      // +
        };

        enum class ConsoleBgColor : uint32_t
        {
            None = 0,
            Black = 40,    // +
            Blue = 44,     // +
            Green = 42,    // +
            // Cyan = 3,
            Red = 41,        // +
            Magenta = 45,    // +
            // Brown        = 6, // No bash Color!
            // LightGray = 7,
            // DarkGray = 8,
            LightBlue = 46,    // +
            // LightGreen = 10,
            // LightCyan = 11,
            // LightRed = 12,
            // LightMagenta = 13,
            Yellow = 43,    // +
            White = 47      // +
        };
#else
        enum class ConsoleColorType : uint32_t
        {
            None = 0,
            Black = 30,
            Blue = 34,
            Green = 32,
            // Cyan = 36,
            Red = 31,
            Magenta = 35,
            // LightGray = 37,
            // DarkGray = 90,
            LightBlue = 94,
            // LightGreen = 92,
            // LightCyan = 96,
            // LightRed = 91,
            // LightMagenta = 95,
            Yellow = 33,
            White = 97,
        };
        typedef ConsoleColorType ConsoleTxtColor;
        typedef ConsoleColorType ConsoleBgColor;
#endif

        enum class ConsoleStreamHandle
        {
            kCout,
            kCerr,
        };

        Logger()
        {
            // #ifdef _WIN32
            //             std::unique_lock Lock(m_Mtx);
            //             std::system("cls");
            // #endif
        }

        void SetColor(ConsoleTxtColor _TXT = ConsoleTxtColor::White, ConsoleBgColor _BG = ConsoleBgColor::None,
                      ConsoleStreamHandle _StreamHandle = ConsoleStreamHandle::kCout);
        void Reset(ConsoleStreamHandle _StreamHandle = ConsoleStreamHandle::kCout);

        template <typename... Args>
        void Trace(std::string_view _LogFormat, Args&&... _Args)
        {
            // std::unique_lock Lock(m_Mtx);
            SetColor(ConsoleTxtColor::White);
            std::cout << Format(s_FormatBase, "DEBUG");
            std::cout << Format(_LogFormat, std::forward<Args>(_Args)...);
            std::cout << std::endl;
            Reset();
        }

        template <typename... Args>
        void Info(std::string_view _LogFormat, Args&&... _Args)
        {
            // std::unique_lock Lock(m_Mtx);
            SetColor(ConsoleTxtColor::Green);
            std::cout << Format(s_FormatBase, "INFO");
            std::cout << Format(_LogFormat, std::forward<Args>(_Args)...);
            std::cout << std::endl;
            Reset();
        }

        template <typename... Args>
        void Warn(std::string_view _LogFormat, Args&&... _Args)
        {
            // std::unique_lock Lock(m_Mtx);
            SetColor(ConsoleTxtColor::Yellow);
            std::cout << Format(s_FormatBase, "WARN");
            std::cout << Format(_LogFormat, std::forward<Args>(_Args)...);
            std::cout << std::endl;
            Reset();
        }

        template <typename... Args>
        void Error(std::string_view _LogFormat, Args&&... _Args)
        {
            // std::unique_lock Lock(m_Mtx);
            SetColor(ConsoleTxtColor::Red, ConsoleBgColor::None, ConsoleStreamHandle::kCerr);
            std::cerr << Format(s_FormatBase, "ERROR");
            std::cerr << Format(_LogFormat, std::forward<Args>(_Args)...);
            std::cerr << std::endl;
            Reset();
        }

        template <typename... Args>
        void Critical(std::string_view _LogFormat, Args&&... _Args)
        {
            // std::unique_lock Lock(m_Mtx);
            SetColor(ConsoleTxtColor::Red, ConsoleBgColor::None, ConsoleStreamHandle::kCerr);
            std::cerr << Format(s_FormatBase, "FATAL");
            std::cerr << Format(_LogFormat, std::forward<Args>(_Args)...);
            std::cerr << std::endl;
            Reset();
        }

        void LogDecorate()
        {
            // std::unique_lock Lock(m_Mtx);
            SetColor(ConsoleTxtColor::LightBlue);
            std::cout << "========================================";
            std::cout << std::endl;
            Reset();
        }

    protected:
        std::mutex m_Mtx;

        static inline constexpr std::string_view s_FormatBase = "[{:<5}]: ";
    };

}    // namespace Vega
