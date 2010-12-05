#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include "npnapi.h"

static int playing = 0;
static const char embed_attrib[] = {
	"width='100' src='http://player.ku6.com/refer/TKmvCmUadJv6rjyG/v.swf' type='application/x-shockwave-flash'"
};

static const char * embed_attrib_p = embed_attrib;
static char embed_attrib_buf[65536];


void callback( GtkWidget *widget,
		gpointer   data )
{
	int id;
	GtkWidget * plugin;

	switch (playing) {
	case 0:
		plugin_Setup(NULL);
		plugin = (GtkWidget *)data;
		id = GDK_WINDOW_XID(plugin->window);
		plugin_New((void*)id, embed_attrib_p);
		playing = 1;
		break;
	case 1:
		plugin_Shutdown();
		playing = 0;
		break;
	default:
		plugin_Test(NULL);
		break;
	}
	//
	//g_print ("Hello again - %s was pressed\n", (gchar *) data);
}

gint delete_event( GtkWidget *widget,
		GdkEvent  *event,
		gpointer   data )
{
	printf("Hello World!\n");
	gtk_main_quit ();
	return FALSE;
}


int main(int argc, char *argv[])
{
	GtkWidget *box, *window;
	GtkWidget *button, *plugin;

	gtk_init(&argc, &argv);

	if (argc > 1) {
		snprintf(embed_attrib_buf, sizeof(embed_attrib_buf), "src='%s' type='application/x-shockwave-flash''", argv[1]);
		embed_attrib_p = embed_attrib_buf;
	}

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "NS Plugin Runner");
	g_signal_connect(G_OBJECT(window), "delete_event",
			G_CALLBACK(delete_event), NULL);
	gtk_container_set_border_width(GTK_CONTAINER(window), 2);
	gtk_window_resize(GTK_WINDOW(window), 610, 510);
	gtk_widget_show(window);

	box = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(window), box);
	gtk_widget_show(box);

	button = gtk_button_new_with_label("run");
	gtk_box_pack_start(GTK_BOX(box), button, FALSE, TRUE, 0);
	gtk_widget_show(button);

	plugin = gtk_socket_new();
	gtk_box_pack_start(GTK_BOX(box), plugin, TRUE, TRUE, 0);
	gtk_widget_show(plugin);

	g_signal_connect(G_OBJECT(button), "clicked",
			G_CALLBACK(callback), (void*)plugin);

	gtk_main ();

	if (playing) {
		plugin_Shutdown();
		playing = 0;
	}

	return 0;
}
