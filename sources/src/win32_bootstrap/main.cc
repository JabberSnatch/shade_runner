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
#pragma warning(disable : 4514) // (Wall) C4514 : unreferenced inline function was removed
#include <cstdlib>
#pragma warning(pop)

#include <iostream>

namespace WXtk {
template <typename T> void unref_param(T&&) {}
}

static const char* wnd_class_name = "ShaderLabMainWindow";
static const char* wnd_title = "Shader Lab";

int CALLBACK WinMain(_In_ HINSTANCE hInstance,
					 _In_ HINSTANCE hPrevInstance,
					 _In_ LPSTR     lpCmdLine,
					 _In_ int       nCmdShow);
LRESULT CALLBACK WndProc(_In_ HWND   hwnd,
						 _In_ UINT   uMsg,
						 _In_ WPARAM wParam,
						 _In_ LPARAM lParam);


int CALLBACK WinMain(_In_ HINSTANCE hInstance,
					 _In_ HINSTANCE hPrevInstance,
					 _In_ LPSTR     lpCmdLine,
					 _In_ int       nCmdShow)
{
	WXtk::unref_param(hPrevInstance);
	WXtk::unref_param(lpCmdLine);
	WXtk::unref_param(nCmdShow);

	WNDCLASS wc{};
	wc.style = CS_OWNDC;
	wc.lpfnWndProc = static_cast<WNDPROC>(WndProc);
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = NULL;
	wc.hCursor = NULL;
	wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(GRAY_BRUSH));
	wc.lpszMenuName = NULL;
	wc.lpszClassName = static_cast<LPCTSTR>(wnd_class_name);
	if (!RegisterClass(&wc))
	{
		MessageBox(NULL, "Window class registration failure", NULL, NULL);
		return 1;
	}

	HWND hWnd = CreateWindow(
		static_cast<LPCTSTR>(wnd_class_name),
		static_cast<LPCTSTR>(wnd_title),
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT,
		1280, 720,
		NULL,
		NULL,
		hInstance,
		NULL);
	if (!hWnd)
	{
		MessageBox(NULL, "Window creation failure", NULL, NULL);
		return 1;
	}

	MSG msg{};
	while (GetMessage(&msg, NULL, 0, 0))
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
	switch(uMsg)
	{
	case WM_PAINT:
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
		break;
	}

	return 0;
}
