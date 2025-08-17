#include "Logger.hpp"
#include <iostream>

#ifdef _WIN32
    #define NOMINMAX
    #define WIN32_LEAN_AND_MEAN
    #include <Windows.h>
#endif

namespace Vega
{

    static std::ostream& getConsoleStreamHandle(Logger::ConsoleStreamHandle _StreamHandle)
    {
        if (_StreamHandle == Logger::ConsoleStreamHandle::kCerr)
        {
            return std::cerr;
        }

        return std::cout;
    }

    void Logger::SetColor(ConsoleTxtColor _TXT, ConsoleBgColor _BG, ConsoleStreamHandle _StreamHandle)
    {
        std::ostream& stream = getConsoleStreamHandle(_StreamHandle);

#ifdef _WIN32
        // HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
        // SetConsoleTextAttribute(hStdOut, (WORD)(((int)_BG << 4) | ((int)_TXT)));

        std::string style =
            ("\u001b[" + std::to_string(static_cast<uint32_t>(_TXT)) + "m") +
            (_BG != ConsoleBgColor::None ? ("\u001b[" + std::to_string(static_cast<uint32_t>(_BG)) + "m") : "");

        stream << style;
#else
        std::string Style = "\033[" + std::to_string((short)_BG) + ";" + std::to_string((short)_TXT) + "m";
        stream << Style;
        // stream << "\033[0m \n"; // To reset atr
#endif
    }

    void Logger::Reset(ConsoleStreamHandle _StreamHandle)
    {
        std::ostream& stream = getConsoleStreamHandle(_StreamHandle);

#ifdef _WIN32
        stream << "\u001b[0m";
#else
        stream << "\033[0m";
#endif
    }

}    // namespace Vega
