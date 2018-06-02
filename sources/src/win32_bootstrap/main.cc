/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Samuel Bourasseau wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

#pragma warning(push)
#pragma warning(disable : 4668) // (Wall) C4668 : undefined macro replaced with 0 for #if/#elif
#include <windows.h>
#pragma warning(pop)

#pragma warning(push)
// (Wall) C4514 : unreferenced inline function was removed
#pragma warning(disable : 4514)
#include <cstdlib>

// (Wall) C4365 : signed/unsigned mismatch
// (Wall) C4571 : catch(...) semantics changed since Visual C++ 7.1; SEH no longer caught
// (Wall) C4625 : cpy ctor implicitly deleted
// (Wall) C4626 : assignment op implicitly deleted
// (Wall) C4774 : not a string literal
// (Wall) C4820 : added padding
// (Wall) C5026 : move ctor implicitly deleted
// (Wall) C5027 : move assignment implicitly deleted
#pragma warning(disable : 4365 4571 4625 4626 4774 4820 5026 5027)
#include <iostream>
#pragma warning(pop)

#include <cassert>
#include <memory>

#include <boost/numeric/conversion/cast.hpp>
#include <GL/glew.h>
#include <GL/wglew.h>

#include "shaderunner/shaderunner.h"

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
LRESULT CALLBACK WndProc(_In_ HWND   hwnd,
						 _In_ UINT   uMsg,
						 _In_ WPARAM wParam,
						 _In_ LPARAM lParam);


// =============================================================================
static char const *wnd_class_name = "ShadeRunnerMainWindow";
static char const *wnd_title = "ShadeRunner";

Win32Handles handles{};
std::unique_ptr<sr::RenderContext> sr_context{};

int CALLBACK WinMain(_In_ HINSTANCE hInstance,
					 _In_ HINSTANCE hPrevInstance,
					 _In_ LPSTR     lpCmdLine,
					 _In_ int       nCmdShow)
{
	WXtk::unref_param(hPrevInstance);
	WXtk::unref_param(lpCmdLine);
	WXtk::unref_param(nCmdShow);

	WNDCLASS wc{ 0 };
	wc.style = CS_OWNDC;
	wc.lpfnWndProc = static_cast<WNDPROC>(WndProc);
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
		handles.hWnd = CreateWindow(
			static_cast<LPCTSTR>(wnd_class_name),
			static_cast<LPCTSTR>(wnd_title),
			WS_OVERLAPPEDWINDOW | WS_VISIBLE,
			CW_USEDEFAULT, CW_USEDEFAULT,
			1280, 720,
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
		pixel_format_desc.cDepthBits = 0; // NOTE: Hope it wasn't too hard finding that one
		pixel_format_desc.cStencilBits = 0;

		const int pixel_format = ChoosePixelFormat(handles.device_context, &pixel_format_desc);
		SetPixelFormat(handles.device_context, pixel_format, &pixel_format_desc);

		handles.gl_context = wglCreateContext(handles.device_context);
		wglMakeCurrent(handles.device_context, handles.gl_context);

		if (glewInit() != GLEW_OK)
			std::cout << "glew failed to initialize." << std::endl;

		constexpr int kProfileBit =
#ifdef SR_GL_COMPATIBILITY_PROFILE
			WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB;
#else
			WGL_CONTEXT_CORE_PROFILE_BIT_ARB;
#endif
		const int gl_attributes[] = {
			WGL_CONTEXT_PROFILE_MASK_ARB, kProfileBit,
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
				wglMakeCurrent(handles.device_context, handles.gl_context);
			}
			else
			{
				std::cout << "wglCreateContext failed" << std::endl;
			}
		}

		std::cout << "GL init complete : " << std::endl;
		std::cout << "OpenGL version : " << glGetString(GL_VERSION) << std::endl;
		std::cout << "Manufacturer : " << glGetString(GL_VENDOR) << std::endl;
		std::cout << "Drivers : " << glGetString(GL_RENDERER) << std::endl;

		if (wglSwapIntervalEXT(1))
		{
			std::cout << "Vsync enabled" << std::endl;
		}
		else
		{
			std::cout << "Could not enable Vsync" << std::endl;
		}

		sr_context.reset(new sr::RenderContext());
		sr_context->LoadFragmentKernel(R"__SR_SS__(
void imageMain(inout vec4 frag_color, vec2 frag_coord) {
	frag_color.xy = frag_coord / vec2(1280.0, 720.0);
	frag_color.a = 1.0;
}
)__SR_SS__");

		wglMakeCurrent(handles.device_context, NULL);
	}

	MSG msg{ 0 };
	while(GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return static_cast<int>(msg.wParam);
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
		assert(sr_context);
		wglMakeCurrent(handles.device_context, handles.gl_context);
		if (!sr_context->RenderFrame())
		{
			ValidateRect(hwnd, NULL);
		}
		::SwapBuffers(handles.device_context);
		wglMakeCurrent(handles.device_context, NULL);
	} break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		result = DefWindowProc(hwnd, uMsg, wParam, lParam);
		break;
	}

	return result;
}
