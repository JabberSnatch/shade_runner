/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Samuel Bourasseau wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

#pragma warning(push)
#pragma warning(disable : 4668)
#include <windows.h>
#pragma warning(pop)
#include <windowsx.h>
#include <io.h>
#include <fcntl.h>
#include <stdio.h>

#pragma warning(push)
#pragma warning(disable : 4514)
#include <cstdlib>

#pragma warning(disable : 4365 4571 4625 4626 4774 4820 5026 5027)
#include <iostream>
#pragma warning(pop)

#include <fstream>

#include <algorithm>
#include <cassert>
#include <functional>
#include <memory>

#include <boost/numeric/conversion/cast.hpp>
#include <GL/glew.h>
#include <GL/wglew.h>

#include "oglbase/error.h"
#include "appbase/layer_mediator.h"

namespace WXtk {
template <typename T> void unref_param(T&&) {}
}


struct Win32Handles
{
	~Win32Handles()
	{
		if (hWnd && device_context)
		{
			if (gl_context)
			{
				std::cout << "deleting gl_context.." << std::endl;
				wglDeleteContext(gl_context);
			}
			std::cout << "releasing device_context.." << std::endl;
			ReleaseDC(hWnd, device_context);
		}
	};
	HWND hWnd;
	HDC	device_context;
	HGLRC gl_context;
};


int CALLBACK WinMain(_In_ HINSTANCE hInstance,
					 _In_ HINSTANCE hPrevInstance,
					 _In_ LPSTR     lpCmdLine,
					 _In_ int       nCmdShow);
LRESULT CALLBACK MinimalWndProc(_In_ HWND   hwnd,
								_In_ UINT   uMsg,
								_In_ WPARAM wParam,
								_In_ LPARAM lParam);
LRESULT CALLBACK WndProc(_In_ HWND   hwnd,
						 _In_ UINT   uMsg,
						 _In_ WPARAM wParam,
						 _In_ LPARAM lParam);


// =============================================================================
static char const *wnd_class_name = "ShadeRunnerMainWindow";
static char const *wnd_title = "ShadeRunner";

constexpr int boot_width = 1280;
constexpr int boot_height = 720;

std::unique_ptr<appbase::LayerMediator> layer_mediator;

Win32Handles handles{};

int CALLBACK WinMain(_In_ HINSTANCE hInstance,
					 _In_ HINSTANCE hPrevInstance,
					 _In_ LPSTR     lpCmdLine,
					 _In_ int       nCmdShow)
{
	WXtk::unref_param(hPrevInstance);
	WXtk::unref_param(lpCmdLine);
	WXtk::unref_param(nCmdShow);

	AllocConsole();
	std::unique_ptr<std::ofstream> console_stdout{};
	{
		HANDLE const h_stdout = GetStdHandle(STD_OUTPUT_HANDLE);
		assert(h_stdout != INVALID_HANDLE_VALUE);
		assert(h_stdout != NULL);
		int const file_desc = _open_osfhandle((intptr_t)h_stdout,
											  _O_TEXT);
		assert(file_desc != -1);
		std::FILE* const file = _fdopen(file_desc, "w");
		assert(file != nullptr);
		console_stdout = std::make_unique<std::ofstream>(file);
	}
	assert(console_stdout != nullptr);
	std::cout.rdbuf(console_stdout->rdbuf());

	WNDCLASS wc{ 0 };
	wc.style = CS_OWNDC;
	wc.lpfnWndProc = static_cast<WNDPROC>(MinimalWndProc);
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = NULL;
	wc.hCursor = NULL;
	wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
	wc.lpszMenuName = NULL;
	wc.lpszClassName = static_cast<LPCTSTR>(wnd_class_name);
	if (!RegisterClass(&wc))
	{
		MessageBox(NULL, "Window class registration failure", NULL, NULL);
		return 1;
	}

	{
		RECT canvas_rect = { 0, 0, boot_width, boot_height };
		DWORD const style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
		AdjustWindowRect(&canvas_rect, style, FALSE);
		int const width = canvas_rect.right - canvas_rect.left;
		int const height = canvas_rect.bottom - canvas_rect.top;

		handles.hWnd = CreateWindowEx(
			NULL,
			static_cast<LPCTSTR>(wnd_class_name),
			static_cast<LPCTSTR>(wnd_title),
			style,
			CW_USEDEFAULT, CW_USEDEFAULT,
			width, height,
			NULL,
			NULL,
			hInstance,
			NULL);
		if (!handles.hWnd)
		{
			MessageBox(NULL, "Window creation failure", NULL, NULL);
			return 1;
		}
	}

	handles.device_context = GetDC(handles.hWnd);

	{
		PIXELFORMATDESCRIPTOR pixel_format_desc = { 0 };
		pixel_format_desc.nSize = sizeof(PIXELFORMATDESCRIPTOR);
		pixel_format_desc.nVersion = 1;
		pixel_format_desc.dwFlags =
#ifndef SR_SINGLE_BUFFERING
			PFD_DOUBLEBUFFER |
#endif
			PFD_SUPPORT_OPENGL |
			PFD_DRAW_TO_WINDOW;
		pixel_format_desc.iPixelType = PFD_TYPE_RGBA;
		pixel_format_desc.cColorBits = 32;
		pixel_format_desc.cDepthBits = 24;
		pixel_format_desc.cStencilBits = 8;

		const int pixel_format = ChoosePixelFormat(handles.device_context, &pixel_format_desc);
		SetPixelFormat(handles.device_context, pixel_format, &pixel_format_desc);

		handles.gl_context = wglCreateContext(handles.device_context);
		wglMakeCurrent(handles.device_context, handles.gl_context);

		if (glewInit() != GLEW_OK)
			std::cout << "glew failed to initialize." << std::endl;

		constexpr int kContextProfile =
#ifdef SR_GL_COMPATIBILITY_PROFILE
			WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB
#else
			WGL_CONTEXT_CORE_PROFILE_BIT_ARB
#endif
			;

		constexpr int kContextFlags =
#ifdef SR_GL_DEBUG_CONTEXT
			WGL_CONTEXT_DEBUG_BIT_ARB
#else
			0
#endif
			;

		const int gl_attributes[] = {
			WGL_CONTEXT_PROFILE_MASK_ARB, kContextProfile,
			WGL_CONTEXT_FLAGS_ARB, kContextFlags,
			0
		};

		if (wglewIsSupported("WGL_ARB_create_context"))
		{
			HGLRC old_context = handles.gl_context;
			handles.gl_context = wglCreateContextAttribsARB(handles.device_context, 0, gl_attributes);
			if (handles.gl_context)
			{
				wglMakeCurrent(NULL, NULL);
				wglDeleteContext(old_context);
			}
			else
			{
				std::cout << "wglCreateContext failed, execution aborted" << std::endl;
				return 1;
			}
		}
	}


	wglMakeCurrent(handles.device_context, handles.gl_context);
	std::cout << "GL init complete : " << std::endl;
	std::cout << "OpenGL version : " << glGetString(GL_VERSION) << std::endl;
	std::cout << "Manufacturer : " << glGetString(GL_VENDOR) << std::endl;
	std::cout << "Drivers : " << glGetString(GL_RENDERER) << std::endl;
	{
		std::cout << "Context flags : ";
		int context_flags = 0;
		glGetIntegerv(GL_CONTEXT_FLAGS, &context_flags);
		if (context_flags & GL_CONTEXT_FLAG_DEBUG_BIT)
			std::cout << "debug, ";
		std::cout << std::endl;
	}

	if (wglSwapIntervalEXT(1))
	{
		std::cout << "Vsync enabled" << std::endl;
	}
	else
	{
		std::cout << "Could not enable Vsync" << std::endl;
	}

#ifdef SR_GL_DEBUG_CONTEXT
	oglbase::DebugMessageControl<> debugMessageControl{};
#endif

	layer_mediator = std::make_unique<appbase::LayerMediator>(
		uibase::Vec2i_t{ boot_width, boot_height },
		appbase::LayerFlag::kShaderunner
		| appbase::LayerFlag::kGizmo
		| appbase::LayerFlag::kImgui
	);

    layer_mediator->SpecialKey(appbase::eKey::kTab, (std::uint8_t)(VK_TAB & 0xff));
    layer_mediator->SpecialKey(appbase::eKey::kLeft, (std::uint8_t)(VK_LEFT & 0xff));
    layer_mediator->SpecialKey(appbase::eKey::kRight, (std::uint8_t)(VK_RIGHT & 0xff));
    layer_mediator->SpecialKey(appbase::eKey::kUp, (std::uint8_t)(VK_UP & 0xff));
    layer_mediator->SpecialKey(appbase::eKey::kDown, (std::uint8_t)(VK_DOWN & 0xff));
    layer_mediator->SpecialKey(appbase::eKey::kPageUp, (std::uint8_t)(VK_PRIOR & 0xff));
    layer_mediator->SpecialKey(appbase::eKey::kPageDown, (std::uint8_t)(VK_NEXT & 0xff));
    layer_mediator->SpecialKey(appbase::eKey::kHome, (std::uint8_t)(VK_HOME & 0xff));
    layer_mediator->SpecialKey(appbase::eKey::kEnd, (std::uint8_t)(VK_END & 0xff));
    layer_mediator->SpecialKey(appbase::eKey::kInsert, (std::uint8_t)(VK_INSERT & 0xff));
    layer_mediator->SpecialKey(appbase::eKey::kDelete, (std::uint8_t)(VK_DELETE & 0xff));
    layer_mediator->SpecialKey(appbase::eKey::kBackspace, (std::uint8_t)(VK_BACK & 0xff));
    layer_mediator->SpecialKey(appbase::eKey::kEnter, (std::uint8_t)(VK_RETURN & 0xff));
    layer_mediator->SpecialKey(appbase::eKey::kEscape, (std::uint8_t)(VK_ESCAPE & 0xff));

	if (__argc > 1)
	{
		layer_mediator->sr_layer_->WatchKernelFile(sr::ShaderStage::kFragment, __argv[1]);
	}
	wglMakeCurrent(handles.device_context, NULL);

	SetWindowLongPtr(handles.hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&WndProc));

	MSG msg{ 0 };
	while(GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	wglMakeCurrent(handles.device_context, handles.gl_context);
    layer_mediator.reset(nullptr);
	wglMakeCurrent(handles.device_context, NULL);

	FreeConsole();
	return static_cast<int>(msg.wParam);
}


LRESULT CALLBACK MinimalWndProc(_In_ HWND   hwnd,
								_In_ UINT   uMsg,
								_In_ WPARAM wParam,
								_In_ LPARAM lParam)
{
	LRESULT result = 1;

	switch(uMsg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		result = DefWindowProc(hwnd, uMsg, wParam, lParam);
		break;
	}

	return result;
}

LRESULT CALLBACK WndProc(_In_ HWND   hwnd,
						 _In_ UINT   uMsg,
						 _In_ WPARAM wParam,
						 _In_ LPARAM lParam)
{
	LRESULT result = 1;

	switch(uMsg)
	{

	case WM_PAINT:
	{
		assert(handles.device_context);
		assert(handles.gl_context);
		assert(layer_mediator);

        wglMakeCurrent(handles.device_context, handles.gl_context);
		if (!layer_mediator->RunFrame())
		{
			ValidateRect(hwnd, NULL);
		}
		::SwapBuffers(handles.device_context);
        wglMakeCurrent(handles.device_context, NULL);
    } break;

	case WM_SIZE:
	{
		int width = LOWORD(lParam);
		int height = HIWORD(lParam);

        wglMakeCurrent(handles.device_context, handles.gl_context);
		if (layer_mediator)
            layer_mediator->ResizeEvent({ width, height });
        wglMakeCurrent(handles.device_context, NULL);
	} break;

    case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
        //case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK:
        //case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK:
    {
        if (::GetCapture() == NULL)
            ::SetCapture(hwnd);

        layer_mediator->MouseDown(true);
    } break;
    case WM_LBUTTONUP:
        //case WM_RBUTTONUP:
        //case WM_MBUTTONUP:
    {
        if (::GetCapture() == hwnd)
            ::ReleaseCapture();

        layer_mediator->MouseDown(false);
    } break;

	case WM_MOUSEMOVE:
	{
		int const pos_x = GET_X_LPARAM(lParam);
		int const pos_y = GET_Y_LPARAM(lParam);
        wglMakeCurrent(handles.device_context, handles.gl_context);
        layer_mediator->MousePos({ pos_x, layer_mediator->state_.screen_size[1] - pos_y });
        layer_mediator->MouseDown((wParam & MK_LBUTTON) != 0u);
        wglMakeCurrent(handles.device_context, NULL);
	} break;

#if 0
    case WM_MOUSEWHEEL:
        io.MouseWheel += (float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA;
		break;
    case WM_MOUSEHWHEEL:
        io.MouseWheelH += (float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA;
		break;
#endif

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYUP:
    {
        std::uint32_t km = 0u;
        km |= ((::GetKeyState(VK_CONTROL) & 0x8000) != 0) ? appbase::fKeyMod::kCtrl : 0u;
        km |= ((::GetKeyState(VK_SHIFT) & 0x8000) != 0) ? appbase::fKeyMod::kShift : 0u;
        km |= ((::GetKeyState(VK_MENU) & 0x8000) != 0) ? appbase::fKeyMod::kAlt : 0u;

        if (wParam < 256)
            layer_mediator->KeyDown((std::uint32_t)wParam, km,
                                    uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN);

		result = CallWindowProc(MinimalWndProc, hwnd, uMsg, wParam, lParam);
    } break;

    case WM_CHAR:
    {
        std::uint32_t km = 0u;
        km |= ((::GetKeyState(VK_CONTROL) & 0x8000) != 0) ? appbase::fKeyMod::kCtrl : 0u;
        km |= ((::GetKeyState(VK_SHIFT) & 0x8000) != 0) ? appbase::fKeyMod::kShift : 0u;
        km |= ((::GetKeyState(VK_MENU) & 0x8000) != 0) ? appbase::fKeyMod::kAlt : 0u;

        // You can also use ToAscii()+GetKeyboardState() to retrieve characters.
        if (wParam > 0 && wParam < 0x10000)
        {
            layer_mediator->KeyDown((std::uint32_t)wParam & 0xff, km, true);
            layer_mediator->KeyDown((std::uint32_t)wParam & 0xff, km, false);
        }

		result = CallWindowProc(MinimalWndProc, hwnd, uMsg, wParam, lParam);
    } break;

	default:
		result = CallWindowProc(MinimalWndProc, hwnd, uMsg, wParam, lParam);
		break;
	}

    return result;
}
