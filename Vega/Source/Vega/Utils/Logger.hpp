
#include "Vega/Core/Base.hpp"

#include "Vega/Utils/utf8.hpp"

#include <iostream>
#include <mutex>
#include <utility>

namespace Vega
{

    class Logger
    {
    public:
#ifdef _WIN32
        enum class ConsoleColorType
        {
            Black = 0,
            Blue = 1,
            Green = 2,
            Cyan = 3,
            Red = 4,
            Magenta = 5,
            // Brown        = 6, // No bash Color!
            LightGray = 7,
            DarkGray = 8,
            LightBlue = 9,
            LightGreen = 10,
            LightCyan = 11,
            LightRed = 12,
            LightMagenta = 13,
            Yellow = 14,
            White = 15
        };
#else
        enum class ConsoleColorType : short
        {
            Black = 30,
            Blue = 34,
            Green = 32,
            Cyan = 36,
            Red = 31,
            Magenta = 35,
            LightGray = 37,
            DarkGray = 90,
            LightBlue = 94,
            LightGreen = 92,
            LightCyan = 96,
            LightRed = 91,
            LightMagenta = 95,
            Yellow = 33,
            White = 97,
        };
#endif
        typedef ConsoleColorType ConsoleTxtColor;
        typedef ConsoleColorType ConsoleBgColor;

        Logger()
        {
            // #ifdef _WIN32
            //             std::unique_lock Lock(m_Mtx);
            //             std::system("cls");
            // #endif
        }

        void SetColor(ConsoleColorType _TXT = ConsoleTxtColor::White, ConsoleColorType _BG = ConsoleBgColor::Black);

        template <typename... Args>
        void Trace(std::string_view _LogFormat, Args&&... _Args)
        {
            std::unique_lock Lock(m_Mtx);
            std::cerr << Format(s_FormatBase, "DEBUG");
            std::cerr << Format(_LogFormat, std::forward<Args>(_Args)...);
            std::cerr << std::endl;
        }

        template <typename... Args>
        void Info(std::string_view _LogFormat, Args&&... _Args)
        {
            std::unique_lock Lock(m_Mtx);
            SetColor(ConsoleTxtColor::Green);
            std::cerr << Format(s_FormatBase, "INFO");
            std::cerr << Format(_LogFormat, std::forward<Args>(_Args)...);
            std::cerr << std::endl;
            SetColor();
        }

        template <typename... Args>
        void Warn(std::string_view _LogFormat, Args&&... _Args)
        {
            std::unique_lock Lock(m_Mtx);
            SetColor(ConsoleTxtColor::Yellow);
            std::cerr << Format(s_FormatBase, "WARN");
            std::cerr << Format(_LogFormat, std::forward<Args>(_Args)...);
            std::cerr << std::endl;
            SetColor();
        }

        template <typename... Args>
        void Error(std::string_view _LogFormat, Args&&... _Args)
        {
            std::unique_lock Lock(m_Mtx);
            SetColor(ConsoleTxtColor::Red);
            std::cerr << Format(s_FormatBase, "ERROR");
            std::cerr << Format(_LogFormat, std::forward<Args>(_Args)...);
            std::cerr << std::endl;
            SetColor();
        }

        template <typename... Args>
        void Critical(std::string_view _LogFormat, Args&&... _Args)
        {
            std::unique_lock Lock(m_Mtx);
            SetColor(ConsoleTxtColor::Red);
            std::cerr << Format(s_FormatBase, "FATAL");
            std::cerr << Format(_LogFormat, std::forward<Args>(_Args)...);
            std::cerr << std::endl;
            SetColor();
        }

        void LogDecorate()
        {
            std::unique_lock Lock(m_Mtx);
            SetColor(ConsoleTxtColor::LightGray);
            std::cerr << "========================================" << std::endl;
            SetColor();
        }

    protected:
        std::mutex m_Mtx;

        static inline constexpr std::string_view s_FormatBase = "[{:<5}]: ";
    };

}    // namespace Vega
