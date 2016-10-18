#include "guihotkeyfield.h"
#include <gdk/gdkkeysyms.h>
#include "x11util.h"

using std::string;

GuiHotKeyField::GuiHotKeyField()
{
}

GuiHotKeyField::~GuiHotKeyField()
{
}

GtkWidget * GuiHotKeyField::widget()
{
    return entry;
}

void GuiHotKeyField::create()
{
    entry = gtk_entry_new();
    g_signal_connect(G_OBJECT(entry), "key-press-event", G_CALLBACK(onKey), this);
    g_signal_connect(G_OBJECT(entry), "key-release-event", G_CALLBACK(onKey), this);
}

gboolean GuiHotKeyField::onKey(GtkWidget *widget, GdkEvent *e, void *data)
{
    GuiHotKeyField * hk = (GuiHotKeyField*) data;
    auto ke = (GdkEventKey*) e;
    if (ke->type == GDK_KEY_PRESS)
    {
        auto kstate = X11KeyListener::getKeyboardState().toString();
        //auto kname = string(gdk_keyval_name(ke->keyval));
        if (kstate == "BackSpace" || kstate == "Delete")
        {
            gtk_entry_set_text((GtkEntry*) hk->entry, "");
            return true;
        }
        gtk_entry_set_text((GtkEntry*) hk->entry, kstate.c_str());
    }
    return true;
}
