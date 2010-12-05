
typedef void *HWND;

int plugin_Shutdown();
int plugin_Player();
int plugin_Setup(HWND);
int plugin_Test(HWND);
int plugin_New(HWND, const char *url);
int plugin_SetWindow(int width, int height);
#ifdef __GDK_H__
void plugin_NetworkNotify(gpointer data, gint fd, GdkInputCondition condition);
#endif

