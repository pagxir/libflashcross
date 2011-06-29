#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <assert.h>
#include <process.h>
#include <winsock2.h>

#include "stdafx.h"
#include "resource.h"

#include "npupp.h"
#include "rpcvm.h"
#include "nputils.h"
#include "npplugin.h"
#include "npp_window.h"
#include "npnetstream.h"

#define WM_DEAD (WM_USER + 0xDEAD)
#define WM_DEAE (WM_USER + 0xDEAE)

#define WIDTH          500
#define HEIGHT         500
#define MAX_LOADSTRING 100

static HINSTANCE hInst;
static TCHAR szTitle[MAX_LOADSTRING];

extern void player_initialize(void);
extern void player_cleanup(void);

static int ProvideRPCCall(HWND hWnd);
static BOOL InitInstance(HINSTANCE, int);
static ATOM  MyRegisterClass(HINSTANCE hInstance);
static LRESULT CALLBACK About(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY WinMain(HINSTANCE hInstance,
		HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	MSG msg;
	HWND hWnd;
	HACCEL hAccelTable;

	WSADATA wsaData;
	WSAStartup(0x101, &wsaData);
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);

	MyRegisterClass(hInstance);
	NPPRegisterEmbed(hInstance);

	hWnd = CreateWindow(NPGeckoWindow, szTitle,
		   	WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN,
		   	CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_W32);

	char *url = GetCommandLine();
	while (url[0] != ' ' || url[1] == ' ') {
		if (url[0] == 0)
			break;
		url++;
	}

	if ( NPPluginLoad("NPSWF32.DLL") ) {
        MessageBox(hWnd, "NPSWF32.DLL ¼ÓÔØÊ§°Ü", "Flash Flv tool", MB_ICONSTOP);
        return -1;
    }

	player_initialize();
	NPPluginInitialize(hWnd);

	NPPluginExec(url + 1);
	ProvideRPCCall(hWnd);

	while (GetMessage(&msg, NULL, 0, 0)) {
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	} 

	NPPluginShutdown();
	player_cleanup();
	WSACleanup();
	return msg.wParam;
}

static ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex, wcex_plugin; 

	wcex.cbSize = sizeof(WNDCLASSEX); 

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = (WNDPROC)WndProc; 
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 4;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, (LPCTSTR)IDI_W32);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(11);
	wcex.lpszMenuName = (LPCSTR)IDC_W32;
	wcex.lpszClassName = NPGeckoWindow;
	wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	return RegisterClassEx(&wcex);
}

int ProvideRPCCall(HWND hWnd)
{
	int error;
	int reuse = 1;
	struct sockaddr_in rpcaddr;
	int ss = socket(AF_INET, SOCK_STREAM, 0);

	rpcaddr.sin_family = AF_INET;
	rpcaddr.sin_port   = htons(8211);
	rpcaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	setsockopt(ss, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse));
	error = bind(ss, (struct sockaddr*)&rpcaddr, sizeof rpcaddr);

	if (error == -1) {
		perror("rpc bind");
		closesocket(ss);
		return -1;
	}

	error = listen(ss, 5);
	if (error == -1) {
		perror("rpc bind");
		closesocket(ss);
		return -1;
	}

	WSAAsyncSelect(ss, hWnd, WM_DEAD, FD_ACCEPT);
	return 0;
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message,
	   	WPARAM wParam, LPARAM lParam)
{
	int fd, len;
	char buf[40960];
	int wmId, wmEvent;
	NPPlugin * plugin;

	switch (message) 
	{
		case WM_CREATE:
			SetWindowLong(hWnd, 0, 0);
			break;
		case WM_SIZE:
			plugin = (NPPlugin*)GetWindowLong(hWnd, 0);
			if (plugin != NULL)
			   	plugin->Resize(LOWORD(lParam), HIWORD(lParam));
			break;
			break;
		case WM_NETN:
			plugin_NetworkNotify(wParam, lParam);
			break;
		case WM_DEAE:
			len = recv(wParam, buf, sizeof buf, 0);
			if (len <= 0 || rpc_dispatch(wParam, buf, len) != 0)
				closesocket(wParam);
			break;
		case WM_DEAD:
			fd = accept(wParam, NULL, NULL);
			WSAAsyncSelect(fd, hWnd, WM_DEAE, FD_READ|FD_CLOSE);
			break;
		case WM_COMMAND:
			wmId    = LOWORD(wParam); 
			wmEvent = HIWORD(wParam); 
			switch (wmId)
			{
				case IDM_ABOUT:
					plugin = (NPPlugin*)GetWindowLong(hWnd, 0);
					if (plugin != NULL)
						plugin->AttachWindow();
					break;
				case IDM_EXIT:
					DestroyWindow(hWnd);
					break;
				default:
					return DefWindowProc(hWnd, message, wParam, lParam);
			}
			break;
		case WM_DESTROY:
			plugin = (NPPlugin*)GetWindowLong(hWnd, 0);
			if (plugin != NULL)
				plugin->DetachWindow();
			if (hWnd == __netscape_hwnd)
			   	PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

static LRESULT CALLBACK About(HWND hDlg, UINT message,
		WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
			return TRUE; 
		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK ||
					LOWORD(wParam) == IDCANCEL) {
				EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
			}
			break;
		default:
			return TRUE;
	}
	return FALSE;
}

#if 0
int main(int argc, char *argv[])
{
	WinMain(NULL, NULL, NULL, SW_SHOW );
	return 0;
}
#endif

