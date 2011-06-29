#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <time.h>
#include <string>
#include <winsock2.h>

#include "stdafx.h"
#include "resource.h"
#include "npupp.h"
#include "npnapi.h"
#include "npplugin.h"
#include "npp_window.h"

HINSTANCE __g_hinstance;

ATOM NPPRegisterEmbed(HINSTANCE hInstance)
{
	WNDCLASSEX wcex; 

	wcex.cbSize = sizeof(WNDCLASSEX); 

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = (WNDPROC)DefWindowProc; 
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = sizeof(LPARAM);
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, (LPCTSTR)IDI_W32);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(11);
	wcex.lpszMenuName = (LPCSTR)NULL;
	wcex.lpszClassName = NPEmbedWindow;
	wcex.hIconSm = LoadIcon(hInstance, (LPCTSTR)IDI_SMALL);
	__g_hinstance = hInstance;

	return RegisterClassEx(&wcex);
}

BOOL SetFullScreen(HWND hWnd)
{
	RECT rect;
	int cX, cY;
	long DlgStyle;
	SetMenu(hWnd, NULL);
	GetWindowRect(hWnd, &rect);
	DlgStyle=::GetWindowLong(hWnd, GWL_STYLE);
	SetWindowLong(hWnd, GWL_STYLE, DlgStyle & (~WS_CAPTION) & (~WS_THICKFRAME));
	cX = GetSystemMetrics(SM_CXSCREEN);
	cY = GetSystemMetrics(SM_CYSCREEN);
	MoveWindow(hWnd, 0, 0, cX, cY, TRUE);
	return TRUE;
}

