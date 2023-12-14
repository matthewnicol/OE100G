#include "base.h"
#include <windows.h>

#include <wingdi.h>
#include <GL/gl.h>
#include <GL/wglext.h>
#include <winuser.h>

// Definitions for function types and function pointers we will load from opengl32.dll
// - wglCreateContextAttribsARB 
// - wglChoosePixelFormatARB

typedef HGLRC WINAPI wglCreateContextAttribsARB_type(HDC hdc,
                                                     HGLRC hShareContext, 
                                                     const int *attribList);
typedef BOOL WINAPI wglChoosePixelFormatARB_type(HDC hdc, 
                                                 const int *piAttribIList, 
                                                 const FLOAT *pfAttribFList, 
                                                 UINT nMaxFormats, 
                                                 int *piFormats, 
                                                 UINT *nNumFormats);

wglCreateContextAttribsARB_type *wglCreateContextAttribsARB = 0;
wglChoosePixelFormatARB_type *wglChoosePixelFormatARB = 0;

// Definitions for attributes for specifying the version we want to wglCreateContextAttribsARB
#define WGL_CONTEXT_MAJOR_VERSION_ARB             0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB             0x2092
#define WGL_CONTEXT_PROFILE_MASK_ARB              0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB          0x00000001

// Definitions for attributes for specifying the pixel format we want to wglChoosePixelFormatARB
#define WGL_DRAW_TO_WINDOW_ARB                    0x2001
#define WGL_ACCELERATION_ARB                      0x2003
#define WGL_SUPPORT_OPENGL_ARB                    0x2010
#define WGL_DOUBLE_BUFFER_ARB                     0x2011
#define WGL_PIXEL_TYPE_ARB                        0x2013
#define WGL_COLOR_BITS_ARB                        0x2014
#define WGL_DEPTH_BITS_ARB                        0x2022
#define WGL_STENCIL_BITS_ARB                      0x2023
#define WGL_FULL_ACCELERATION_ARB                 0x2027
#define WGL_TYPE_RGBA_ARB                         0x202B

// OpenGL Loader

void *OS_loadOpenGLFunction(const char *name) {
    void *p = (void *)wglGetProcAddress(name);
    if (p == 0 || (p == (void*)0x1) || (p == (void*)0x2) || (p == (void*)0x3) || (p == (void*)-1)) {
        // Functions added to OpenGL after version 1.1 can't be loaded with wglGetProcAddress.
        // Use GetProcAddress if wglGetProcAddress doesn't work.
        HMODULE module = LoadLibraryA("opengl32.dll");
        p = (void *)GetProcAddress(module, name);
    }
    return p;
}

typedef struct OS_Window {
    HINSTANCE hInstance;
    HGLRC hRC;
    HDC hDC;
    HWND hwnd;
    RECT windowRect;
    WNDCLASS wndCls;
    S32 width;
    S32 height;
    S32 bitsPerPixel;
    S32 isFullScreen;
} OS_Window;

// For now, we have one active window, and its attributes are stored in this global struct. 
// If this ever changes, a lot of this code will need to be restructured.
OS_Window OS_activeWindow;

// Window Event Handling

// Our custom windows system event message handler function
LRESULT CALLBACK SystemEventCallback(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
        // WINDOW LIFECYCLE
        case WM_CLOSE: { DestroyWindow(hwnd); } break;
        case WM_DESTROY: { PostQuitMessage(0); } break;
        
        default:
            // Defer to default handler
            break;
    }
	return DefWindowProc(hwnd, message, wparam, lparam);
}

void NBASE_OSWindow_handleEvents() {
    // Our engine will call this function once per frame (for now... this might change later).
    // - Consolidate game engine state (coming soon!)
    // - Process all pending windows messages

    MSG message;
    while (PeekMessage(&message, 0, 0, 0, PM_REMOVE)) {
        if (message.message == WM_QUIT) {
            exit(1);                   // FOR NOW
            break;
        } else {
            DispatchMessage(&message);
        }
    }
}

// Window Creation

HGLRC startupAndBindOpenGLToWindow(HDC real_dc);
HWND createWindow(HINSTANCE inst, S32 width, S32 height, S32 bitsPerPixel, S32 isFullScreen);

void NBASE_OSWindow_startup(S32 width, S32 height) {
    S32 bitsPerPixel = 32;
    S32 isFullScreen = 0;
    HINSTANCE hInstance = GetModuleHandle(0);
    HWND window = createWindow(hInstance, width, height, bitsPerPixel, isFullScreen);
    HDC gldc = GetDC(window);
    startupAndBindOpenGLToWindow(gldc);
    
    OS_activeWindow.hInstance = hInstance;
    OS_activeWindow.hwnd = window;
    OS_activeWindow.width = width;
    OS_activeWindow.height = height;
    OS_activeWindow.bitsPerPixel = bitsPerPixel;
    OS_activeWindow.isFullScreen = isFullScreen;
    OS_activeWindow.hRC = wglCreateContext(gldc);
    OS_activeWindow.hDC = gldc;
    
    UpdateWindow(window);
    GetWindowRect(OS_activeWindow.hwnd, &OS_activeWindow.windowRect);

    // Select crosshair cursor
    HCURSOR cursor = LoadCursor(0, IDC_CROSS);
    SetCursor(cursor);
    UpdateWindow(window);
    ShowWindow(window, 1);
}

HGLRC startupAndBindOpenGLToWindow(HDC bindToDC) {
    {
        // Create a dummy OpenGL context so we can get access to wglChoosePixelFormatARB / wglCreateContextAttribsARB.
        // We use this to create a proper OpenGL context below.
        WNDCLASSA legacyWindowClass = {
            .style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
            .lpfnWndProc = DefWindowProcA,
            .hInstance = GetModuleHandle(0),
            .lpszClassName = "LEGACY_WINDOW_CLASS",
        };

        ASSERT_WITH_MESSAGE(RegisterClassA(&legacyWindowClass), 
            "Bootstrapping OpenGL: Failed to register Legacy OpenGL Window Class.");

        HWND legacyOpenGLWindow = CreateWindowExA(
            0,
            legacyWindowClass.lpszClassName,
            "LEGACY WINDOW",
            0,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            0,
            0,
            legacyWindowClass.hInstance,
            0);

        ASSERT_WITH_MESSAGE(legacyOpenGLWindow, "Failed to create dummy OpenGL window.");

        HDC legacyDeviceContext = GetDC(legacyOpenGLWindow);

        // Specify a dummy pixel format, and then choose the closest match
        PIXELFORMATDESCRIPTOR desiredPixelFormat = {
            .nSize = sizeof(desiredPixelFormat),
            .nVersion = 1,
            .iPixelType = PFD_TYPE_RGBA,
            .dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
            .cColorBits = 32,
            .cAlphaBits = 8,
            .iLayerType = PFD_MAIN_PLANE,
            .cDepthBits = 24,
            .cStencilBits = 8,
        };

        S32 pixelFormat = ChoosePixelFormat(legacyDeviceContext, &desiredPixelFormat);
        ASSERT_WITH_MESSAGE(pixelFormat, 
            "Failed to find a suitable pixel format.");
        ASSERT_WITH_MESSAGE(
            SetPixelFormat(legacyDeviceContext, pixelFormat, &desiredPixelFormat), 
            "Failed to set the pixel format.");

        HGLRC dummy_context = wglCreateContext(legacyDeviceContext);
        ASSERT_WITH_MESSAGE(dummy_context, 
            "Failed to create a dummy OpenGL rendering context.");
        ASSERT_WITH_MESSAGE(wglMakeCurrent(legacyDeviceContext, dummy_context), 
            "Failed to activate dummy OpenGL rendering context.");

        wglCreateContextAttribsARB = (wglCreateContextAttribsARB_type*)wglGetProcAddress("wglCreateContextAttribsARB");
        wglChoosePixelFormatARB = (wglChoosePixelFormatARB_type*)wglGetProcAddress("wglChoosePixelFormatARB");

        wglMakeCurrent(legacyDeviceContext, 0);
        wglDeleteContext(dummy_context);
        ReleaseDC(legacyOpenGLWindow, legacyDeviceContext);
        DestroyWindow(legacyOpenGLWindow);
    }
    
    // Now we can choose a pixel format the modern way, using wglChoosePixelFormatARB.
    int desiredPixelFormatAttributes[] = {
        WGL_DRAW_TO_WINDOW_ARB,     GL_TRUE,
        WGL_SUPPORT_OPENGL_ARB,     GL_TRUE,
        WGL_DOUBLE_BUFFER_ARB,      GL_TRUE,
        WGL_ACCELERATION_ARB,       WGL_FULL_ACCELERATION_ARB,
        WGL_PIXEL_TYPE_ARB,         WGL_TYPE_RGBA_ARB,
        WGL_COLOR_BITS_ARB,        32,
        WGL_DEPTH_BITS_ARB,        24,
        WGL_STENCIL_BITS_ARB,      8,
        0
    };

    S32 pixelFormat;
    U32 numFormats;
    wglChoosePixelFormatARB(bindToDC, desiredPixelFormatAttributes, 0, 1, &pixelFormat, &numFormats);
    ASSERT_WITH_MESSAGE(numFormats, 
        "There are no available pixel formats to select from.");

    PIXELFORMATDESCRIPTOR pfd;
    DescribePixelFormat(bindToDC, pixelFormat, sizeof(pfd), &pfd);
    ASSERT_WITH_MESSAGE(
        SetPixelFormat(bindToDC, pixelFormat, &pfd), 
        "Failed to set an OpenGL 4.6 compatible pixel format.");

    // Specify the version of OpenGL that we want to use
    int desiredOpenGLVersionAttributes[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
        WGL_CONTEXT_MINOR_VERSION_ARB, 6,
        WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        0,
    };

    HGLRC openglRenderingContext = wglCreateContextAttribsARB(bindToDC, 0, desiredOpenGLVersionAttributes);
    
    ASSERT_WITH_MESSAGE(openglRenderingContext, 
        "Failed to create OpenGL context.");
    
    ASSERT_WITH_MESSAGE(
        wglMakeCurrent(bindToDC, openglRenderingContext), 
        "Failed to activate OpenGL rendering context.");

    return openglRenderingContext;
}

HWND createWindow(HINSTANCE inst, S32 width, S32 height, S32 bitsPerPixel, S32 isFullScreen) {
    WNDCLASSA window_class = {
        .style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
        .lpfnWndProc = SystemEventCallback,
        .hInstance = inst,
        .hCursor = LoadCursor(0, IDC_ARROW),
        .hbrBackground = 0,
        .lpszClassName = "WGL_fdjhsklf",
    };

    ASSERT_WITH_MESSAGE(
        RegisterClassA(&window_class), 
        "Failed to register window.");
    
    // Specify a desired width and height, then adjust the rect so the window's client area will be that size.
    RECT rect = {.right = width, .bottom = height, .top = 0, .left = 0 };

    // Get our default monitor and find its dimensions.    
    HMONITOR monitor = MonitorFromPoint((const POINT){0,0}, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO monitorInfo = { sizeof(monitorInfo) };
    GetMonitorInfo(monitor, &monitorInfo);
    
    // Position our window such that it is in the middle of the default monitor's screen
    if (rect.right > monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left)
        rect.right = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
    if (rect.bottom > monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top)
        rect.bottom = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;
    
    if (rect.right < monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left) {
        rect.left = (monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left - rect.right) / 2;
        rect.right += rect.left;
    }

    if (rect.bottom < monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top) {
        rect.top = (monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top - rect.bottom) / 2;
        rect.bottom += rect.top;
    }

    DWORD windowStyle = WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_TILEDWINDOW;
    
    AdjustWindowRect(&rect, WS_EX_TOPMOST, 0);
    
    HWND window = CreateWindowEx(
        WS_EX_TOPMOST,
        window_class.lpszClassName,
        NBASE_WINDOW_NAME_QUOTED,
        windowStyle,
        0,
        0,
        rect.right - rect.left,
        rect.bottom - rect.top,
        0,
        0,
        inst,
        0);

    ASSERT_WITH_MESSAGE(window, 
        "Failed to create window.");
    
    SetWindowPos(window, 
                 0, 
                 rect.left, 
                 rect.top, 
                 0, 
                 0, 
                 SWP_SHOWWINDOW | SWP_NOSIZE);
    ShowWindow(window, SW_MAXIMIZE);
    SetForegroundWindow(window);
    SetFocus(window);

    return window;
}

void NBASE_OSWindow_swapBuffers() {
    SwapBuffers(OS_activeWindow.hDC);
}