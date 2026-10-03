#include "WinTitleBar.hpp"

#include "Platform/Platform.hpp"

#if defined(VEGA_PLATFORM_WINDOWS_DESKTOP)

    #include <GLFW/glfw3.h>
    #define GLFW_EXPOSE_NATIVE_WIN32
    #include <GLFW/glfw3native.h>

    #include <dwmapi.h>
    #include <shellapi.h>
    #include <windowsx.h>

// NOTE: UNICODE isn't defined for Vega, so wide-char WinAPI functions are called explicitly. GLFW creates a unicode
// window and subclassing it with an ANSI window procedure makes the system convert WM_CHAR to the ANSI code page.

namespace Vega
{

    namespace
    {

        const wchar_t* const kTitleBarProp = L"VegaWinTitleBar";
        const wchar_t* const kOriginalWndProcProp = L"VegaWinTitleBarOriginalWndProc";

        // DWMWA_USE_IMMERSIVE_DARK_MODE, missing in old Windows SDKs
        constexpr DWORD kDwmUseImmersiveDarkMode = 20;

        // Gap left at the screen edge with an auto-hide taskbar: otherwise a maximized window covers the whole
        // monitor and the taskbar can't be shown by the mouse
        constexpr LONG kAutoHideTaskbarGap = 2;

        // Size of the (invisible) resize frame of a window with WS_THICKFRAME
        int GetFrameThickness(HWND _HWnd)
        {
            UINT dpi = GetDpiForWindow(_HWnd);
            return GetSystemMetricsForDpi(SM_CYSIZEFRAME, dpi) + GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
        }

        void AdjustMaximizedClientRectForAutoHideTaskbar(HWND _HWnd, RECT& _ClientRect)
        {
            APPBARDATA stateData = { .cbSize = sizeof(APPBARDATA) };
            if ((SHAppBarMessage(ABM_GETSTATE, &stateData) & ABS_AUTOHIDE) == 0)
            {
                return;
            }

            MONITORINFO monitorInfo = { .cbSize = sizeof(MONITORINFO) };
            if (!GetMonitorInfoW(MonitorFromWindow(_HWnd, MONITOR_DEFAULTTONEAREST), &monitorInfo))
            {
                return;
            }

            auto isAutoHideTaskbarOnEdge = [&monitorInfo](UINT _Edge) {
                APPBARDATA edgeData = { .cbSize = sizeof(APPBARDATA), .uEdge = _Edge, .rc = monitorInfo.rcMonitor };
                return SHAppBarMessage(ABM_GETAUTOHIDEBAREX, &edgeData) != 0;
            };

            if (isAutoHideTaskbarOnEdge(ABE_TOP))
            {
                _ClientRect.top += kAutoHideTaskbarGap;
            }
            else if (isAutoHideTaskbarOnEdge(ABE_BOTTOM))
            {
                _ClientRect.bottom -= kAutoHideTaskbarGap;
            }
            else if (isAutoHideTaskbarOnEdge(ABE_LEFT))
            {
                _ClientRect.left += kAutoHideTaskbarGap;
            }
            else if (isAutoHideTaskbarOnEdge(ABE_RIGHT))
            {
                _ClientRect.right -= kAutoHideTaskbarGap;
            }
        }

        LRESULT CALLBACK TitleBarWndProc(HWND _HWnd, UINT _Msg, WPARAM _WParam, LPARAM _LParam)
        {
            WNDPROC originalWndProc = reinterpret_cast<WNDPROC>(GetPropW(_HWnd, kOriginalWndProcProp));
            WinTitleBar* titleBar = static_cast<WinTitleBar*>(GetPropW(_HWnd, kTitleBarProp));

            if (!originalWndProc)
            {
                return DefWindowProcW(_HWnd, _Msg, _WParam, _LParam);
            }
            if (!titleBar)
            {
                return CallWindowProcW(originalWndProc, _HWnd, _Msg, _WParam, _LParam);
            }

            switch (_Msg)
            {
                case WM_NCCALCSIZE: {
                    if (_WParam != TRUE)
                    {
                        break;
                    }

                    NCCALCSIZE_PARAMS* params = reinterpret_cast<NCCALCSIZE_PARAMS*>(_LParam);
                    LONG originalTop = params->rgrc[0].top;

                    // The system computes the standard frame: left, right and bottom borders stay native
                    LRESULT result = CallWindowProcW(originalWndProc, _HWnd, _Msg, _WParam, _LParam);
                    if (result != 0)
                    {
                        return result;
                    }

                    // Remove the caption and the top border (it's emulated by WM_NCHITTEST in the client area)
                    params->rgrc[0].top = originalTop;
                    if (IsZoomed(_HWnd))
                    {
                        // A maximized window is larger than the monitor work area by the frame thickness
                        params->rgrc[0].top += GetFrameThickness(_HWnd);
                        AdjustMaximizedClientRectForAutoHideTaskbar(_HWnd, params->rgrc[0]);
                    }

                    return 0;
                }
                case WM_NCHITTEST: {
                    // Native borders (left, right, bottom and their corners) are handled by the system
                    LRESULT hit = CallWindowProcW(originalWndProc, _HWnd, _Msg, _WParam, _LParam);
                    if (hit != HTCLIENT)
                    {
                        return hit;
                    }

                    POINT point = { GET_X_LPARAM(_LParam), GET_Y_LPARAM(_LParam) };
                    ScreenToClient(_HWnd, &point);

                    switch (titleBar->HitTest({ static_cast<float>(point.x), static_cast<float>(point.y) }))
                    {
                        case TitleBarHit::kResizeTop: return HTTOP;
                        case TitleBarHit::kCaption: return HTCAPTION;
                        default: return HTCLIENT;
                    }
                }
                case WM_NCACTIVATE: {
                    // -1: don't repaint the non-client area, otherwise the system caption may flash on activation
                    return CallWindowProcW(originalWndProc, _HWnd, _Msg, _WParam, -1);
                }
            }

            return CallWindowProcW(originalWndProc, _HWnd, _Msg, _WParam, _LParam);
        }

        void UpdateWindowFrame(HWND _HWnd)
        {
            SetWindowPos(_HWnd, nullptr, 0, 0, 0, 0,
                         SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        }

    }    // namespace

    WinTitleBar::WinTitleBar(GLFWwindow* _Window) : GLFWTitleBar(_Window)
    {
        HWND hWnd = glfwGetWin32Window(_Window);

        // Window props instead of GWLP_USERDATA: the procedure must be able to forward messages even if the title
        // bar is destroyed while someone else subclassed the window after us
        LONG_PTR originalWndProc = GetWindowLongPtrW(hWnd, GWLP_WNDPROC);
        SetPropW(hWnd, kOriginalWndProcProp, reinterpret_cast<HANDLE>(originalWndProc));
        SetPropW(hWnd, kTitleBarProp, static_cast<HANDLE>(this));
        SetWindowLongPtrW(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(TitleBarWndProc));

        // Dark window border on Windows 11
        BOOL isDarkMode = TRUE;
        DwmSetWindowAttribute(hWnd, kDwmUseImmersiveDarkMode, &isDarkMode, sizeof(isDarkMode));

        UpdateWindowFrame(hWnd);
    }

    WinTitleBar::~WinTitleBar()
    {
        HWND hWnd = glfwGetWin32Window(m_Window);

        RemovePropW(hWnd, kTitleBarProp);
        if (GetWindowLongPtrW(hWnd, GWLP_WNDPROC) == reinterpret_cast<LONG_PTR>(TitleBarWndProc))
        {
            SetWindowLongPtrW(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(GetPropW(hWnd, kOriginalWndProcProp)));
            RemovePropW(hWnd, kOriginalWndProcProp);
        }

        UpdateWindowFrame(hWnd);
    }

    float WinTitleBar::GetResizeBorderWidth() const
    {
        return static_cast<float>(GetFrameThickness(glfwGetWin32Window(m_Window)));
    }

}    // namespace Vega

#endif
