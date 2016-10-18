#include "applet.h"
#include "application.h"
#include <assert.h>

std::map<MatePanelApplet*, Applet*> Applet::vInstances;
Applet * Applet::lastInstance = NULL;

Applet::Applet(MatePanelApplet *mateApplet)
{
    this->mateApplet = mateApplet;
    vInstances[mateApplet] = this;
}

Applet::~Applet()
{
    auto it = vInstances.find(this->mateApplet);
    assert(it != vInstances.end());
    vInstances.erase(it);

}

GtkWidget * Applet::getAppletBox()
{
    if (lastInstance == NULL)
        return NULL;
    return lastInstance->hbox;
}

bool Applet::isRunning()
{
    return Applet::vInstances.size() > 0;
}

gboolean Applet::onButtonPress(GtkWidget *widget, GdkEventButton *event, Applet * appinst)
{
    lastInstance = appinst;
    if (event->type == GDK_BUTTON_PRESS && event->button == 1) //left button
    {
        app->onPressAppletButton();
        return TRUE;
    }
    return FALSE;
}

gboolean Applet::onMouseInOut(GtkWidget *widget, GdkEventButton *event, Applet * appinst)
{
    auto state = event->type == GDK_ENTER_NOTIFY ? GTK_STATE_SELECTED : GTK_STATE_NORMAL;
    gtk_widget_set_state(appinst->container, state);
    return FALSE;
}

void Applet::setButton(const string& label, const string& iconPath)
{
    for (auto inst : vInstances)
    {
        auto appinst = inst.second;
        if (appinst->icon != NULL)
        {
            gtk_widget_destroy(appinst->icon);
            appinst->icon = NULL;
        }

        if (iconPath != "")
        {
            appinst->icon = gtk_image_new_from_file(iconPath.c_str());
            gtk_box_pack_start(GTK_BOX(appinst->hboxi), appinst->icon, 0, 0, 0);
            gtk_widget_show(appinst->icon);
        }
        string txt = label;
        if (txt.empty() && iconPath == "")
            txt = _("Menu");
        gtk_label_set_markup((GtkLabel*) appinst->label, txt.c_str());
        gtk_widget_show(appinst->hboxi);
    }
}

void Applet::appletDestroy(MatePanelApplet *mateApplet, Applet * appinst)
{
    delete appinst;
    printf("\napplet instance %p deleted. (%lu)", mateApplet, vInstances.size());
    lastInstance = NULL;
    auto it = vInstances.begin();
    if (it != vInstances.end())
        lastInstance = it->second;
    else
    {
        app->quit();
        printf("\nquit");
    }
}

void Applet::init()
{
    mate_panel_applet_set_flags(mateApplet, MATE_PANEL_APPLET_EXPAND_MINOR);
    mate_panel_applet_set_background_widget(mateApplet, GTK_WIDGET(mateApplet));
    hbox = gtk_hbox_new(FALSE, 0);
    hboxi = gtk_hbox_new(FALSE, 0);
    container = gtk_event_box_new();

    g_signal_connect(G_OBJECT(container), "button-press-event", G_CALLBACK(onButtonPress), this);
    g_signal_connect(G_OBJECT(container), "enter-notify-event", G_CALLBACK(onMouseInOut), this);
    g_signal_connect(G_OBJECT(container), "leave-notify-event", G_CALLBACK(onMouseInOut), this);
    g_signal_connect(G_OBJECT(mateApplet), "destroy", G_CALLBACK(appletDestroy), this);

    label = gtk_label_new("Menu");

    gtk_container_add(GTK_CONTAINER(mateApplet), (GtkWidget*) container);
    gtk_container_add(GTK_CONTAINER(container), hbox);
    gtk_box_pack_start(GTK_BOX(hbox), hboxi, 0, 0, 2);
    gtk_box_pack_start(GTK_BOX(hbox), label, 0, 0, 2);
    gtk_widget_show_all((GtkWidget*) mateApplet);
    lastInstance = this;

    if (vInstances.size() == 1)
        app->start(true);

    app->onNewAppletInstance();
    printf("\nnew applet instance %p initialized.", mateApplet);
}

gboolean Applet::appletFactory(MatePanelApplet * mateApplet, const char* iid, gpointer data)
{
    gboolean retval = FALSE;
    if (!g_strcmp0(iid, Application::A1MENU_APPLET_NAME))
    {
        (new Applet(mateApplet))->init();
        retval = TRUE;
    }
    return retval;
}

#define main(c,v) Applet::appletMain(c,v)
MATE_PANEL_APPLET_OUT_PROCESS_FACTORY( //
        Application::A1MENU_APPLET_FACTORY,//
        PANEL_TYPE_APPLET,//
        Installer::A1MENU_APPLET_DESC,//
        Applet::appletFactory, NULL)
#undef main
