#include "guisearchbox.h"
#include "sutl.h"
#include "application.h"
#include "guiitem.h"
#include "application.h"
#include "x11util.h"
#include <vector>
#include <assert.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms-compat.h>
#include <gdk/gdkx.h>

SearchBox::SearchBox()
{
}

SearchBox::~SearchBox()
{
}

GtkWidget * SearchBox::widget()
{
    return box;
}

void SearchBox::create(GuiWindow * wnd)
{
    this->guiwnd = wnd;
    box = gtk_hbox_new(false, 0);
    entry = gtk_entry_new();
    gtk_widget_set_size_request (entry,50,-1);

    GtkWidget * slab = gtk_label_new(res.labSearch.c_str());
    gtk_misc_set_alignment(GTK_MISC(slab), 1, .5); //range (0.0 - 1.0) left <-> right
    gtk_misc_set_padding(GTK_MISC(slab), 10, 0);
    auto pix = app->loader->getCachedPixbuf(res.iconSearchBoxName, res.iconSearchBoxSize);
    if (pix)
    {
        gtk_entry_set_icon_from_pixbuf((GtkEntry*) entry, GTK_ENTRY_ICON_SECONDARY, pix);
        g_object_unref(pix);
    }

    gtk_box_pack_start((GtkBox*) box,slab, 0, 0, 0);
    gtk_box_pack_start((GtkBox*) box, entry, 1, 1, 0);
    gtk_widget_show_all(box);

    g_signal_connect_after(G_OBJECT(entry), "notify", G_CALLBACK(onSearchBoxChanged), this);
    g_signal_connect(G_OBJECT(entry), "key-press-event", G_CALLBACK(onKey), this);

}

GtkEntry * SearchBox::getEntry()
{
    return (GtkEntry*) entry;
}

void SearchBox::setText(string txt)
{
    gtk_entry_set_text((GtkEntry*) entry, txt.c_str());
}

string SearchBox::getText()
{
    return string(gtk_entry_get_text((GtkEntry*) entry));
}

gboolean SearchBox::onSearchBoxChanged(GtkWidget *widget, GdkEvent *event, void *data)
{
    CHECK_IS_HANDLER_BLOCKED();

    SearchBox * sb = (SearchBox*) data;
    string text = sb->getText();
    if (sb->prevSearchText != text)
    {
        sb->prevSearchText = text;
        sb->guiwnd->appList->refresh(text);
    }
    return TRUE;
}


gboolean SearchBox::onKey(GtkWidget *widget, GdkEvent *e, void *data)
{
    CHECK_IS_HANDLER_BLOCKED();

    SearchBox * sb = (SearchBox*) data;
    auto ke = (GdkEventKey*) e;
    if (ke->keyval == GDK_Escape)
    {
        gtk_entry_set_text((GtkEntry*) sb->entry, "");
        return TRUE;
    }
    if (ke->keyval == GDK_Down)
    {
        sb->guiwnd->appList->goNext();
        return TRUE;
    }
    if (ke->keyval == GDK_Return)
    {
        auto it = sb->guiwnd->appList->getSelectedItem();
        if (it != NULL)
        {
            app->runMenuProcess(it);
            return TRUE;
        }
    }
    return FALSE;
}
