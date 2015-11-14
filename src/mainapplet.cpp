#include "mainwindow.h"
#include "ctx.h"
#include <unistd.h>
#include <assert.h>

#undef signals

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include <mate-panel-applet.h>
#include <mate-panel-applet-gsettings.h>

#define A1MENU_APPLET_NAME    "A1Menu_AppletID"
#define A1MENU_APPLET_DESC    "a1menu applet"
#define A1MENU_APPLET_FACTORY "A1Menu"
//#define A1MENU_APPLET_SCHEMA  "org.mate.panel.applet.a1menu"

struct applet_data
{
	MatePanelApplet *applet = NULL;
	GtkWidget *label = NULL;
	GtkWidget *container = NULL;
	GtkWidget *hbox = NULL;
	GtkWidget *hboxi = NULL;
	GtkWidget *icon = NULL;

	GdkColor locolor;
	GdkColor hicolor;
	int statelo = 0;
	int statehi = 1;
	int instance = 0;
//	GSettings *settings = NULL;
};

struct main_data
{
	int argc;
	char **argv;
	MainWindow * mainQtWindow;
	int instcount = 0;
	applet_data * app = NULL;

} maindt;

gboolean callback_handler_button_press(GtkWidget *widget, GdkEventButton *event, GtkWidget *dataObj)
{
	if (event->type == GDK_BUTTON_PRESS && event->button == 1) //left button
	{
		if (maindt.mainQtWindow == NULL)
		{
			printf("callback_handler_button_press: applet not initialized!");
			assert(maindt.mainQtWindow!=NULL);
			exit(1);
		}
		maindt.mainQtWindow->sendShowEvent();
		return TRUE;
	}
	return FALSE;
}

applet_data * getapp()
{
	return maindt.app;
}

gboolean callback_handler_highlight(GtkWidget *widget, GdkEventButton *event, void *dataObj)
{
	int * state = (int *) dataObj;
	if (*state)
		gtk_widget_set_state(getapp()->container, GTK_STATE_PRELIGHT);
	else
		gtk_widget_set_state(getapp()->container, GTK_STATE_NORMAL);
	return FALSE;
}

void panel_set_menu_button(const char * label, const char * iconPath)
{
	auto app = getapp();
	if (app->hboxi == NULL)
	{
		printf("panel_set_menu_button: applet not initialized!");
		//assert(app->hboxi!=NULL);
		exit(1);
	}
	if (app->icon != NULL)
	{
		gtk_widget_destroy(app->icon);
		app->icon = NULL;
	}

	if (strlen(iconPath) > 0)
	{
		app->icon = gtk_image_new_from_file(iconPath);
		gtk_box_pack_start(GTK_BOX(app->hboxi), app->icon, 0, 0, 0);
		gtk_widget_show(app->icon);
	}
	gtk_widget_show(app->hboxi);
	char *str = g_strdup_printf(label);
	gtk_label_set_markup((GtkLabel*) app->label, str);
	g_free(str);
}

static void menu_applet_destroy(MatePanelApplet *applet, void * data)
{
	applet_data * app = (applet_data*) data;
	delete app;
}

int hicol(int c16)
{
	double cd = c16 * 1.1;
	return cd > (double) 0xFFFF ? 0xFFFF : (int) cd;
}

void highlight(GdkColor& col)
{
	col.red = hicol((int) col.red);
	col.green = hicol((int) col.green);
	col.blue = hicol((int) col.blue);
}

static gboolean memu_applet_init(MatePanelApplet * applet)
{
	printf("memu_applet_init\n");

	applet_data * app = new applet_data;
	if (maindt.instcount == 1)
		maindt.app = app;
	app->instance = maindt.instcount;
	//app->settings = mate_panel_applet_settings_new(applet, (gchar*)A1MENU_APPLET_SCHEMA);
	mate_panel_applet_set_flags(applet, MATE_PANEL_APPLET_EXPAND_MINOR);
	mate_panel_applet_set_background_widget(applet, GTK_WIDGET(applet));
	app->applet = applet;
	app->hbox = gtk_hbox_new(FALSE, 0);
	app->hboxi = gtk_hbox_new(FALSE, 0);
	app->container = gtk_event_box_new();

	g_signal_connect(G_OBJECT (app->container), "button_press_event", G_CALLBACK (callback_handler_button_press), NULL);
	g_signal_connect(G_OBJECT (app->container), "enter-notify-event", G_CALLBACK (callback_handler_highlight), &(app->statehi));
	g_signal_connect(G_OBJECT (app->container), "leave-notify-event", G_CALLBACK (callback_handler_highlight), &(app->statelo));
	g_signal_connect(G_OBJECT (applet), "destroy", G_CALLBACK (menu_applet_destroy), app);

	if (maindt.instcount <= 1)
		app->label = gtk_label_new("Menu");
	else
		app->label = gtk_label_new("menu is a single-instance applet");

	gtk_box_pack_start(GTK_BOX(app->hbox), app->hboxi, 0, 0, 2);
	gtk_box_pack_start(GTK_BOX(app->hbox), app->label, 0, 0, 2);
	gtk_container_add(GTK_CONTAINER(app->container), app->hbox);
	gtk_container_add(GTK_CONTAINER(applet), (GtkWidget*) app->container);

	gtk_widget_show_all((GtkWidget*) applet);
	ctx::thread_data.isGtkInitialized = 1;

	return TRUE;
}

static gboolean a1menu_factory(MatePanelApplet * applet, const char* iid, gpointer data)
{
	gboolean retval = FALSE;
	if (!g_strcmp0(iid, A1MENU_APPLET_NAME))
	{
		maindt.instcount++;
		if (maindt.instcount > 1)
		{
			printf("warning: a1menu is a single-instance applet\n");
		}
		retval = memu_applet_init(applet);
	}
	return retval;
}

void setMainArguments(int argc, char ** argv, MainWindow * wnd)
{
	maindt.argc = argc;
	maindt.argv = argv;
	maindt.mainQtWindow = wnd;
}

#define main(c,v) applet_main(c,v)
MATE_PANEL_APPLET_OUT_PROCESS_FACTORY(A1MENU_APPLET_FACTORY, PANEL_TYPE_APPLET, A1MENU_APPLET_DESC, a1menu_factory, NULL)
#undef main

/* content of the MATE_PANEL_APPLET_OUT_PROCESS_FACTORY macro
 *
 int applet_main(int argc, char* argv[])
 {
 GOptionContext* context;
 GError* error;
 int retval;

 do
 {
 } while (0);

 context = g_option_context_new("");
 g_option_context_add_group(context, gtk_get_option_group((!(0))));

 error = 0;
 if (!g_option_context_parse(context, &argc, &argv, &error))
 {
 if (error)
 {
 g_printerr("Cannot parse arguments: %s.\n", error->message);
 g_error_free(error);
 } else
 {
 g_printerr("Cannot parse arguments.\n");
 }
 g_option_context_free(context);
 return 1;
 }

 gtk_init(&argc, &argv);

 retval = mate_panel_applet_factory_main("A1Menu_FactoryID", (!(0)), (mate_panel_applet_get_type()), a1menu_factory, 0);
 g_option_context_free(context);

 return retval;
 }
 */

#define signals protected

