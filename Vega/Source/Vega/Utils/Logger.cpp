#include "Logger.hpp"

#ifdef _WIN32
    #define NOMINMAX
    #define WIN32_LEAN_AND_MEAN
    #include <Windows.h>
#endif

namespace Vega
{

    void Logger::SetColor(ConsoleColorType _TXT, ConsoleColorType _BG)
    {
#ifdef _WIN32
        // HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
        // SetConsoleTextAttribute(hStdOut, (WORD)(((int)_BG << 4) | ((int)_TXT)));

        // TODO: Use this
        std::string Style =
            "\u001b[" + std::to_string(31) + "m";    //+ "\u001b[" + std::to_string(1) + ";" + std::to_string(31) + "m";
        std::cerr << Style;

#else
        std::string Style = "\033[" + std::to_string((short)_BG) + ";" + std::to_string((short)_TXT) + "m";
        std::cerr << Style;
        // std::cerr << "\033[0m \n"; // To reset atr
#endif
    }

    void Logger::Reset() { std::cerr << "\033[0m"; }

}    // namespace Vega
