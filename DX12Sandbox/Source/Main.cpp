#include "pch.h"

#include "Camera.h"
#include "Renderer.h"

CRenderer* Renderer = nullptr;

LRESULT CALLBACK WindowProcess(HWND HWnd, UINT Message, WPARAM WParam, LPARAM LParam)
{
	switch (Message)
	{
	case WM_KEYDOWN:
		if (WParam == VK_ESCAPE)
		{
			DestroyWindow(HWnd);
		}
		if (WParam == 'Z' && Renderer)
		{
			Renderer->SceneCamera->MoveForward(1.0f);
		}
		if (WParam == 'S' && Renderer)
		{
			Renderer->SceneCamera->MoveForward(-1.0f);
		}
		if (WParam == 'D' && Renderer)
		{
			Renderer->SceneCamera->MoveRight(1.0f);
		}
		if (WParam == 'Q' && Renderer)
		{
			Renderer->SceneCamera->MoveRight(-1.0f);
		}
		if (WParam == 'A' && Renderer)
		{
			Renderer->SceneCamera->MoveUp(1.0f);
		}
		if (WParam == 'E' && Renderer)
		{
			Renderer->SceneCamera->MoveUp(-1.0f);
		}

	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}

	return DefWindowProc(HWnd, Message, WParam, LParam);
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	/* Create the Window Class */
	WNDCLASSEX WindowClass;
	WindowClass.cbSize = sizeof(WNDCLASSEX);
	WindowClass.style = CS_HREDRAW | CS_VREDRAW;
	WindowClass.cbClsExtra = 0;
	WindowClass.cbWndExtra = 0;

	WindowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	WindowClass.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	
	HICON WindowIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1));
	WindowClass.hIcon = WindowIcon;
	WindowClass.hIconSm = WindowIcon;

	WCHAR WindowClassName[MAX_NAME_STRING];
	LoadString(GetModuleHandle(NULL), ProgramName, WindowClassName, MAX_NAME_STRING);
	WindowClass.lpszClassName = WindowClassName;
	WindowClass.lpszMenuName = nullptr;

	WindowClass.hInstance = GetModuleHandle(NULL);
	WindowClass.lpfnWndProc = WindowProcess;

	RegisterClassEx(&WindowClass);

	Renderer = new CRenderer();

	/* Initialize the Window Class*/
	HWND HWnd = CreateWindow(WindowClassName, WindowClassName, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, Renderer->WindowWidth, Renderer->WindowHeight, nullptr, nullptr, GetModuleHandle(NULL), nullptr);
	if (!HWnd)
	{
		MessageBox(0, L"Failed to create the window! Aborting!", 0, 0);
		return 1;
	}
	ShowWindow(HWnd, SW_SHOW);

	Renderer->HWindow = HWnd;
	
	// Initialize Direct3D
	if (!Renderer->InitD3D())
	{
		MessageBox(0, L"Failed to init Direct3D! Aborting!", L"Error", MB_OK);
		Renderer->Cleanup();
		return 1;
	}	

	/* Handle Messages */
	MSG Message = { 0 };
	while (Message.message != WM_CLOSE && Renderer->bRunning)
	{
		if (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&Message);
			DispatchMessage(&Message);
		}
		else
		{
			Renderer->Update();
			Renderer->Render();
		}
	}

	// Wait for the GPU to finish, then cleanup
	Renderer->WaitForPreviousFrame();
	CloseHandle(Renderer->FenceEvent);

	return 0;
}