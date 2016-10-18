#include "guitoolbar.h"
#include "application.h"
#include "guiitem.h"
#include "guiparametersform.h"
#include "guisearchbox.h"
#include <assert.h>

GuiToolbar::GuiToolbar()
{
}

GuiToolbar::~GuiToolbar()
{
}

GtkWidget * GuiToolbar::widget()
{
    return alignment;
}

void GuiToolbar::addButton(GtkWidget ** button, const std::string& iconName, const std::string& label)
{
    int iconSize = CFGI("toolbar_icon_size");
    auto icon = app->loader->getCachedIcon(CFG(iconName), iconSize);
    *button = gtk_button_new();
    auto b = *button;
    gtk_button_set_image((GtkButton*) b, icon);
    gtk_box_pack_start((GtkBox*) toolbar, b, 0, 0, 2);
    g_signal_connect(G_OBJECT (b), "clicked", G_CALLBACK (onButtonClicked), (GuiToolbar* )this);
    gtk_widget_set_tooltip_markup(b, label.c_str());

}

void GuiToolbar::create(GuiWindow * wnd)
{
    this->guiwnd = wnd;

    alignment = gtk_alignment_new(0.0, 0.0, 1.0, 1.0);
    gtk_alignment_set_padding((GtkAlignment *) alignment, 0, 0, 5, 5);
    toolbar = gtk_hbox_new(0, 0);
    gtk_container_add(GTK_CONTAINER(alignment), toolbar);

    gtk_box_pack_end((GtkBox*) toolbar, wnd->searchBox->widget(), 1, 1, 2);

    addButton(&btnSettings, "icon_toolbar_settings", res.toolSettings);
    addButton(&btnReload, "icon_toolbar_reload", res.toolReload);
    addButton(&btnLogout, "icon_toolbar_logout", res.toolLogout);
    addButton(&btnLockScreen, "icon_toolbar_lockscreen", res.toolLockScreen);
    addButton(&btnShutdown, "icon_toolbar_shutdown", res.toolShutdown);

}

gboolean GuiToolbar::onButtonClicked(GtkWidget *widget, void *data)
{
    CHECK_IS_HANDLER_BLOCKED();

    GuiToolbar * toolbar = (GuiToolbar*) data;

    string cmd;
    if (widget == toolbar->btnShutdown)
    {
        cmd = CFG("command_shutdown");
    }
    if (widget == toolbar->btnLockScreen)
    {
        cmd = CFG("command_lockscreen");
    }
    if (widget == toolbar->btnLogout)
    {
        cmd = CFG("command_logout");
    }

    if (widget == toolbar->btnReload)
    {
        app->reload(false);
    }
    if (widget == toolbar->btnSettings)
    {
        GuiParametersForm::run();
    }
    if (!cmd.empty())
    {
        app->runMenuProcess(NULL,cmd);
    }

    return TRUE;
}
