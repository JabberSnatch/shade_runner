/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Samuel Bourasseau wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

#define _SCL_SECURE_NO_WARNINGS

#pragma warning(push)
#pragma warning(disable : 4668) // (Wall) C4668 : undefined macro replaced with 0 for #if/#elif
#include <windows.h>
#pragma warning(pop)
#include <windowsx.h>

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

#include <algorithm>
#include <cassert>
#include <memory>

#include <boost/numeric/conversion/cast.hpp>
#include <imgui.h>
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
	if (wglSwapIntervalEXT(1))
	{
		std::cout << "Vsync enabled" << std::endl;
	}
	else
	{
		std::cout << "Could not enable Vsync" << std::endl;
	}

	sr_context.reset(new sr::RenderContext());
	if (__argc > 1)
	{
		sr_context->WatchFKernelFile(__argv[1]);
	}
	sr_context->SetResolution(boot_width, boot_height);
	wglMakeCurrent(handles.device_context, NULL);

	{
		ImGuiIO &io = ImGui::GetIO();
		io.KeyMap[ImGuiKey_Tab] = VK_TAB;
		io.KeyMap[ImGuiKey_LeftArrow] = VK_LEFT;
		io.KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;
		io.KeyMap[ImGuiKey_UpArrow] = VK_UP;
		io.KeyMap[ImGuiKey_DownArrow] = VK_DOWN;
		io.KeyMap[ImGuiKey_PageUp] = VK_PRIOR;
		io.KeyMap[ImGuiKey_PageDown] = VK_NEXT;
		io.KeyMap[ImGuiKey_Home] = VK_HOME;
		io.KeyMap[ImGuiKey_End] = VK_END;
		io.KeyMap[ImGuiKey_Insert] = VK_INSERT;
		io.KeyMap[ImGuiKey_Delete] = VK_DELETE;
		io.KeyMap[ImGuiKey_Backspace] = VK_BACK;
		io.KeyMap[ImGuiKey_Space] = VK_SPACE;
		io.KeyMap[ImGuiKey_Enter] = VK_RETURN;
		io.KeyMap[ImGuiKey_Escape] = VK_ESCAPE;
		io.KeyMap[ImGuiKey_A] = 'A';
		io.KeyMap[ImGuiKey_C] = 'C';
		io.KeyMap[ImGuiKey_V] = 'V';
		io.KeyMap[ImGuiKey_X] = 'X';
		io.KeyMap[ImGuiKey_Y] = 'Y';
		io.KeyMap[ImGuiKey_Z] = 'Z';
	}

	SetWindowLongPtr(handles.hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&WndProc));

	MSG msg{ 0 };
	while(GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	wglMakeCurrent(handles.device_context, handles.gl_context);
	sr_context.reset(nullptr);
	wglMakeCurrent(handles.device_context, NULL);

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
	ImGuiIO &io = ImGui::GetIO();

	switch(uMsg)
	{
	case WM_PAINT:
	{
		assert(handles.device_context);
		assert(handles.gl_context);
		assert(sr_context);

		io.KeyCtrl = (::GetKeyState(VK_CONTROL) & 0x8000) != 0;
		io.KeyShift = (::GetKeyState(VK_SHIFT) & 0x8000) != 0;
		io.KeyAlt = (::GetKeyState(VK_MENU) & 0x8000) != 0;
		io.KeySuper = false;

		{
			ImGuiStyle &style = ImGui::GetStyle();
			style.FrameRounding = 0.f;
			style.WindowRounding = 1.f;
			style.ScrollbarRounding = 0.f;
			style.GrabRounding = 2.f;
		}

		constexpr int kTextBufferSize = 512;
		static bool show_demo_window = false;
		static bool show_file_selection_window = true;
		static char fkernel_path_buffer[kTextBufferSize] = "";
		ImGui::NewFrame();
		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("Windows"))
			{
				ImGui::MenuItem("Demo", nullptr, &show_demo_window);
				if (ImGui::MenuItem("File Selection", nullptr, &show_file_selection_window) && false)
				{
					std::string const &fkernel_path = sr_context->GetFKernelPath();
					assert(fkernel_path.size() <= kTextBufferSize);
					std::copy(fkernel_path.cbegin(), fkernel_path.cend(), &fkernel_path_buffer[0]);
				}
				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}
		if (show_demo_window)
			ImGui::ShowDemoWindow(&show_demo_window);
		if (show_file_selection_window)
		{
			if (ImGui::Begin("File Selection", &show_file_selection_window, 0))
			{
				std::string const &fkernel_path = sr_context->GetFKernelPath();
				assert(fkernel_path.size() <= kTextBufferSize);
				std::copy(fkernel_path.cbegin(), fkernel_path.cend(), &fkernel_path_buffer[0]);
				bool const state = ImGui::InputText("Path",
													fkernel_path_buffer,
													kTextBufferSize,
													ImGuiInputTextFlags_EnterReturnsTrue);
				if (state)
				{
					std::cout << "imgui input received" << std::endl;
					wglMakeCurrent(handles.device_context, handles.gl_context);
					sr_context->WatchFKernelFile(fkernel_path_buffer);
					wglMakeCurrent(handles.device_context, NULL);
				}
			}
			ImGui::End();
		}

		ImGui::EndFrame();
		ImGui::Render();

		wglMakeCurrent(handles.device_context, handles.gl_context);
		if (!sr_context->RenderFrame())
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
		if (sr_context)
		{
			wglMakeCurrent(handles.device_context, handles.gl_context);
			sr_context->SetResolution(width, height);
			wglMakeCurrent(handles.device_context, NULL);
		}
	} break;
    case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK:
    case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK:
    {
        int button = 0;
        if (uMsg == WM_LBUTTONDOWN || uMsg == WM_LBUTTONDBLCLK) button = 0;
        if (uMsg == WM_RBUTTONDOWN || uMsg == WM_RBUTTONDBLCLK) button = 1;
        if (uMsg == WM_MBUTTONDOWN || uMsg == WM_MBUTTONDBLCLK) button = 2;
        if (!ImGui::IsAnyMouseDown() && ::GetCapture() == NULL)
            ::SetCapture(hwnd);
        io.MouseDown[button] = true;
    } break;
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
    {
        int button = 0;
        if (uMsg == WM_LBUTTONUP) button = 0;
        if (uMsg == WM_RBUTTONUP) button = 1;
        if (uMsg == WM_MBUTTONUP) button = 2;
        io.MouseDown[button] = false;
        if (!ImGui::IsAnyMouseDown() && ::GetCapture() == hwnd)
            ::ReleaseCapture();
    } break;
	case WM_MOUSEMOVE:
	{
		int const pos_x = GET_X_LPARAM(lParam);
		int const pos_y = GET_Y_LPARAM(lParam);
		io.MousePos = ImVec2(boost::numeric_cast<float>(pos_x),
							 boost::numeric_cast<float>(pos_y));
		io.MouseDown[0] = (wParam & MK_LBUTTON) != 0u;
		io.MouseDown[1] = (wParam & MK_RBUTTON) != 0u;
		io.MouseDown[2] = (wParam & MK_MBUTTON) != 0u;
	} break;
    case WM_MOUSEWHEEL:
        io.MouseWheel += (float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA;
		break;
    case WM_MOUSEHWHEEL:
        io.MouseWheelH += (float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA;
		break;
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        if (wParam < 256)
            io.KeysDown[wParam] = 1;
		result = CallWindowProc(MinimalWndProc, hwnd, uMsg, wParam, lParam);
		break;
    case WM_KEYUP:
    case WM_SYSKEYUP:
        if (wParam < 256)
            io.KeysDown[wParam] = 0;
		result = CallWindowProc(MinimalWndProc, hwnd, uMsg, wParam, lParam);
		break;
    case WM_CHAR:
        // You can also use ToAscii()+GetKeyboardState() to retrieve characters.
        if (wParam > 0 && wParam < 0x10000)
            io.AddInputCharacter((unsigned short)wParam);
		result = CallWindowProc(MinimalWndProc, hwnd, uMsg, wParam, lParam);
		break;
	default:
		result = CallWindowProc(MinimalWndProc, hwnd, uMsg, wParam, lParam);
		break;
	}

	return result;
}
