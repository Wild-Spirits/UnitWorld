#include "MacOSTitleBar.hpp"

#include "Platform/Platform.hpp"

#if defined(VEGA_PLATFORM_MACOS_DESKTOP)

    #import <Cocoa/Cocoa.h>

    #include <GLFW/glfw3.h>
    #define GLFW_EXPOSE_NATIVE_COCOA
    #include <GLFW/glfw3native.h>

namespace Vega
{

    namespace
    {

        NSWindow* GetNSWindow(GLFWwindow* _Window) { return (NSWindow*)glfwGetCocoaWindow(_Window); }

        // Respects "System Settings > Desktop & Dock > Double-click a window's title bar to"
        void PerformTitleBarDoubleClickAction(NSWindow* _NSWindow)
        {
            NSString* action = [[NSUserDefaults standardUserDefaults] stringForKey:@"AppleActionOnDoubleClick"];
            if ([action isEqualToString:@"Minimize"])
            {
                [_NSWindow performMiniaturize:nil];
            }
            else if (![action isEqualToString:@"None"])
            {
                [_NSWindow performZoom:nil];
            }
        }

    }    // namespace

    MacOSTitleBar::MacOSTitleBar(GLFWwindow* _Window) : GLFWTitleBar(_Window)
    {
        NSWindow* nsWindow = GetNSWindow(_Window);
        nsWindow.styleMask |= NSWindowStyleMaskFullSizeContentView;
        nsWindow.titlebarAppearsTransparent = YES;
        nsWindow.titleVisibility = NSWindowTitleHidden;
    }

    MacOSTitleBar::~MacOSTitleBar()
    {
        NSWindow* nsWindow = GetNSWindow(m_Window);
        nsWindow.styleMask &= ~NSWindowStyleMaskFullSizeContentView;
        nsWindow.titlebarAppearsTransparent = NO;
        nsWindow.titleVisibility = NSWindowTitleVisible;
    }

    void MacOSTitleBar::OnUpdate()
    {
        if (!m_IsWaitingButtonRelease || ([NSEvent pressedMouseButtons] & 1) != 0)
        {
            return;
        }

        m_IsWaitingButtonRelease = false;

        if (glfwGetMouseButton(m_Window, GLFW_MOUSE_BUTTON_LEFT) != GLFW_PRESS)
        {
            return;
        }

        // Send the consumed mouse up to GLFW, otherwise the button stays pressed for the client
        @autoreleasepool
        {
            NSWindow* nsWindow = GetNSWindow(m_Window);
            NSEvent* event = [NSEvent mouseEventWithType:NSEventTypeLeftMouseUp
                                                location:[nsWindow mouseLocationOutsideOfEventStream]
                                           modifierFlags:0
                                               timestamp:[[NSProcessInfo processInfo] systemUptime]
                                            windowNumber:nsWindow.windowNumber
                                                 context:nil
                                             eventNumber:0
                                              clickCount:1
                                                pressure:0.0f];
            [NSApp postEvent:event atStart:NO];
        }
    }

    void MacOSTitleBar::OnMouseButton(int _Button, int _Action)
    {
        if (_Button != GLFW_MOUSE_BUTTON_LEFT || _Action != GLFW_PRESS)
        {
            return;
        }

        if (HitTest(GetCursorPos()) != TitleBarHit::kCaption)
        {
            return;
        }

        // GLFW calls the callback while handling the event, so it's still the current one
        NSEvent* event = [NSApp currentEvent];
        if (event == nil || event.type != NSEventTypeLeftMouseDown)
        {
            return;
        }

        NSWindow* nsWindow = GetNSWindow(m_Window);
        if (event.clickCount == 2)
        {
            PerformTitleBarDoubleClickAction(nsWindow);
            return;
        }

        [nsWindow performWindowDragWithEvent:event];
        m_IsWaitingButtonRelease = true;
    }

    float MacOSTitleBar::GetNativeButtonsWidth() const
    {
        NSWindow* nsWindow = GetNSWindow(m_Window);
        if (nsWindow.styleMask & NSWindowStyleMaskFullScreen)
        {
            // Buttons are shown in the sliding system title bar in full screen
            return 0.0f;
        }

        NSButton* closeButton = [nsWindow standardWindowButton:NSWindowCloseButton];
        NSButton* zoomButton = [nsWindow standardWindowButton:NSWindowZoomButton];
        if (closeButton == nil || zoomButton == nil)
        {
            return 0.0f;
        }

        NSRect closeFrame = [closeButton convertRect:closeButton.bounds toView:nil];
        NSRect zoomFrame = [zoomButton convertRect:zoomButton.bounds toView:nil];
        // The same margin after the buttons as the system one before them
        return static_cast<float>(NSMaxX(zoomFrame) + NSMinX(closeFrame));
    }

}    // namespace Vega

#endif
