#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <windows.h>

#include "npupp.h"
#include "nputils.h"
#include "npplugin.h"
#include "npp_window.h"

#define NPF_ATTACH_WINDOW 1
#define NPF_DETACH_WINDOW 2
#define NPF_INITED_WINDOW 4

extern HINSTANCE __g_hinstance;
static int __gen_major = (GetTickCount() / 1000);

NPPlugin::NPPlugin(HWND hWnd)
{
	memset(this, 0, sizeof(NPP_t));
	m_parent = hWnd;
	m_embed = NULL;
	m_flags = 0;
	m_minor = 0;
	m_major = __gen_major++;
}

NPPlugin::~NPPlugin()
{
	int combine = (NPF_INITED_WINDOW | NPF_DETACH_WINDOW);
	if ((m_flags & combine) == NPF_INITED_WINDOW) {
		assert (m_parent != NULL);
		DestroyWindow(m_parent);
	}
}

char * NPPlugin::NewStreamName(char *buf, size_t len)
{
	snprintf(buf, len, "%04d%c", m_major, 'A'+m_minor++);
	return buf;
}

int NPPlugin::AttachWindow(void)
{
	fprintf(stderr, "NPPlugin::AttachWindow\n");
	if (m_parent && m_embed) {
		assert (m_embed != NULL);
		if (m_flags & NPF_ATTACH_WINDOW) {
		   	m_flags &= ~NPF_ATTACH_WINDOW;
		   	ShowWindow(m_embed, SW_HIDE);
		} else {
		   	m_flags |= NPF_ATTACH_WINDOW;
		   	ShowWindow(m_embed, SW_SHOW);
		}
	   	return 0;
	}
	return -1;
}

int NPPlugin::SetWindow(void)
{
	RECT wrect;
	NPRect rect;
	NPWindow & win = m_window;
	if ((m_flags & NPF_INITED_WINDOW) == 0) {
		m_flags |= NPF_INITED_WINDOW;
		NPGeckoWindowNew();
	}

	fprintf(stderr, "NPPlugin::SetWindow: %p\n", m_embed);
	int combine = (NPF_INITED_WINDOW | NPF_DETACH_WINDOW);
	if ((m_flags & combine) != (NPF_INITED_WINDOW))
		return __pluginfunc.setwindow(this, NULL);

	assert (m_embed && m_parent);

	GetClientRect(m_parent, &wrect);
	rect.top = wrect.top;
	rect.left = wrect.left;
	rect.right = wrect.right;
	rect.bottom = wrect.bottom;

	win.window = m_embed;
	win.x = 8;
	win.y = 8;
	win.width = wrect.right;
	win.height = wrect.bottom;
	win.clipRect = rect;
	win.type = NPWindowTypeWindow;
	return __pluginfunc.setwindow(this, &win);
}

int NPPlugin::DetachWindow(void)
{
	if (m_flags & NPF_INITED_WINDOW) {
		m_flags |= NPF_DETACH_WINDOW;
		m_embed = NULL;
	}
	return 0;
}

int NPPlugin::Resize(size_t width, size_t height)
{
	NPRect rect;
	NPWindow & win = m_window;
	if ((m_flags & NPF_INITED_WINDOW) == 0) {
		m_flags |= NPF_INITED_WINDOW;
		NPGeckoWindowNew();
	}
   
	int combine = (NPF_ATTACH_WINDOW | NPF_DETACH_WINDOW);
	if ((m_flags & combine) != (NPF_ATTACH_WINDOW))
		return -1;

	assert (m_embed && m_parent);
	MoveWindow(m_embed, 0, 0, width, height, 0);

	rect.top = 0;
	rect.left = 0;
	rect.right = width;
	rect.bottom = height;

	win.window = m_embed;
	win.x = 8;
	win.y = 8;
	win.width = width;
	win.height = height;
	win.clipRect = rect;
	win.type = NPWindowTypeWindow;
	return __pluginfunc.setwindow(this, &win);
}

int NPPlugin::NPGeckoWindowNew(void)
{
	HWND hPlugin;
	ULONG style = WS_CHILD|WS_MAXIMIZE|WS_VISIBLE;

	if (m_parent == NULL) {
	   	m_parent = CreateWindow(NPGeckoWindow, "plugin",
			   	WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN, 
				CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL,
			   	__g_hinstance, this);
	   	ShowWindow(m_parent, SW_MINIMIZE);
		style |= WS_VISIBLE;
	}
	assert (m_parent != NULL);

   	m_embed = CreateWindow(NPEmbedWindow,
		   	NPEmbedWindow, style, 8, 8, 480, 400,
		   	m_parent, HMENU(0x1982), __g_hinstance, NULL);

	SetWindowLong(m_parent, 0, (ULONG)this);
	return 0;
}

void DoTest(NPP pp);
void NPPlugin::Test(void)
{
	DoTest(this);
}

