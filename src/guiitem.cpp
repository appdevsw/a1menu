#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms-compat.h>
#include "guiitem.h"
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sstream>
#include <assert.h>
#include "application.h"
#include "toolkit.h"
#include <unistd.h>
#include "loader.h"
#include "sutl.h"
#include "application.h"
#include "properties.h"
#include "guisearchbox.h"

using namespace std;

GuiItem::GuiItem(GuiList * list)
{
    static int serialCounter = 0;
    serial = ++serialCounter;
    this->list = list;
}

GuiItem::~GuiItem()
{
}

GtkWidget * GuiItem::widget()
{
    return container;
}

GtkWidget * GuiItem::buttonWidget()
{
    return button;
}

void GuiItem::create(GuiWindow * wnd)
{
    this->guiwnd = wnd;

    int height;
    string vpadding;
    //printf("\nitem type %s %s",rec.title.c_str(),rec.itemtype.c_str());
    if (rec.itemtype == res.itemPlace)
    {
        iconSize = CFGI("place_icon_size");
        height = CFGI("place_item_height");
        vpadding = CFG("place_item_padding");
    } else if (rec.itemtype == res.itemCategory)
    {
        iconSize = CFGI("category_icon_size");
        height = CFGI("category_item_height");
        vpadding = CFG("category_item_padding");
    } else
    {
        iconSize = CFGI("app_icon_size");
        height = CFGI("app_item_height");
        vpadding = CFG("app_item_padding");
    }

    container = gtk_vbox_new(false, 0);
    button = gtk_button_new();
    gtk_container_add(GTK_CONTAINER(container), button);
    auto hboxInButton = gtk_hbox_new(false, 0);
    gtk_container_add(GTK_CONTAINER(button), hboxInButton);

#define ACCFGB(x) (isApp()?CFGBOOL(string("app_")+x):CFGBOOL(string("category_")+x))

    if (ACCFGB("item_show_icon"))
    {
        icon = app->loader->getCachedIcon(rec.iconName, iconSize);
    }

    if (ACCFGB("item_show_text") || icon == NULL)
    {
        string title;
        if (CFGBOOL("use_generic_names"))
            title = rec.locgenname == "" ? rec.genname : rec.locgenname;
        if (title.empty())
            title = locTitle();

        labTitle = gtk_label_new(title.c_str());
        if (list->isScrollable())
            gtk_label_set_ellipsize((GtkLabel*) labTitle, PANGO_ELLIPSIZE_END);
    }

    if (isApp() && labTitle && ACCFGB("item_show_comment") && rec.itemtype != res.itemPlace)
    {
        labComment = gtk_label_new(locComment().c_str());
        gtk_label_set_ellipsize((GtkLabel*) labComment, PANGO_ELLIPSIZE_END);
    }

    if (labTitle)
    {
        if (labComment)
        {
            makeLabel(labTitle, 1, 0);
            makeLabel(labComment, 0, 1);
        } else
            makeLabel(labTitle, 0, 0);
    }

    auto vboxLabel = gtk_vbox_new(false, 0);
    if (labTitle)
        gtk_box_pack_start((GtkBox*) vboxLabel, labTitle, 1, 1, 0);
    if (labComment)
        gtk_box_pack_start((GtkBox*) vboxLabel, labComment, 1, 1, 0);
    if (icon)
        gtk_box_pack_start((GtkBox*) hboxInButton, icon, 0, 0, 0);
    gtk_box_pack_start((GtkBox*) hboxInButton, gtk_label_new(" "), 0, 0, 0);
    gtk_box_pack_start((GtkBox*) hboxInButton, vboxLabel, 1, 1, 0);

    setTooltip();

    g_signal_connect(G_OBJECT(button), "enter-notify-event", G_CALLBACK(onMouseEnter), this);
    g_signal_connect(G_OBJECT(button), "leave-notify-event", G_CALLBACK(onMouseLeave), this);
    g_signal_connect(G_OBJECT(button), "button-press-event", G_CALLBACK(onButton), this);
    g_signal_connect(G_OBJECT(button), "button-release-event", G_CALLBACK(onButton), this);
    g_signal_connect(G_OBJECT(button), "key-press-event", G_CALLBACK(onButton), this);
    g_signal_connect(G_OBJECT(button), "focus-in-event", G_CALLBACK(onFocus), this);

    if (isApp() && !rec.fname.empty())
    {
        g_signal_connect(G_OBJECT(button), "drag-data-get", G_CALLBACK(onDragEnd), this);
        static const GtkTargetEntry targetentries[1] = { { strdup("text/uri-list"), 0, 0 }, };
        gtk_drag_source_set(button, GDK_BUTTON1_MASK, targetentries, G_N_ELEMENTS(targetentries), GDK_ACTION_COPY);
    }

#ifdef GTK3
    {
        string style = "";
        if (!vpadding.empty() && vpadding != res.labDefault)
        {
            style = "padding-top: " + vpadding + "px; padding-bottom: " + vpadding + "px;border-width: 1px; ";
        }
        style = " * {" + style + "transition: none;}";
        auto prov = gtk_css_provider_new();
        gtk_css_provider_load_from_data(prov, style.c_str(), -1, NULL);
        auto context = gtk_widget_get_style_context((GtkWidget*) button);
        gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(prov), GTK_STYLE_PROVIDER_PRIORITY_USER);
        g_object_unref(prov);
    }
#endif

    if (height > 0)
    {
        gtk_widget_set_size_request(button, -1, height);
    }
    toFlat();
}

void GuiItem::addSpace(int y)
{
#ifdef GTK3
    auto sp = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
#else
    auto sp = gtk_hseparator_new();
#endif

    gtk_widget_set_size_request(sp, -1, y);
    gtk_box_pack_start((GtkBox*) container, sp, 0, 0, 0);

}

void GuiItem::makeLabel(GtkWidget * label, bool bold, bool italic)
{
    string s;
    s += (string) (bold ? "<b>" : "");
    s += (italic ? "<i>" : "");
    s += gtk_label_get_text((GtkLabel*) label);
    s += (italic ? "</i>" : "");
    s += (string) (bold ? "</b>" : "");
    string labelText = sutl::prepareMarkup(s);
    gtk_label_set_markup(GTK_LABEL(label), labelText.c_str());
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
}

void GuiItem::show(bool enable)
{
    if (enable)
    {
        gtk_widget_show(widget());
        toFlat();
    } else
        gtk_widget_hide(widget());
}

void GuiItem::toButton()
{
    gtk_button_set_relief((GtkButton*) button, GTK_RELIEF_NORMAL);
}

void GuiItem::toFlat()
{
    gtk_button_set_relief((GtkButton*) button, GTK_RELIEF_NONE);
}

gboolean GuiItem::onTimer(gpointer data)
{

    CHECK_IS_HANDLER_BLOCKED();

    GuiItem::TimerRec *td = (GuiItem::TimerRec*) data;
    if(!app->isTimerTokenValid(td->timerToken))
    {
        printf("\nGuiItem::onTimer canceled.  (%i)",td->timerToken);
        delete td;
        return FALSE;
    }

    if (td->type == td->SET_CURRENT_ROW)
    {
        if (td->count == td->item->list->changeCounter)
        {
            td->item->list->setCurrent(td->item,true);
        }
    }
    if (td->type == td->TOOLTIP && td->item->mouseIn)
    {
        gtk_widget_set_tooltip_markup(td->item->button, td->item->tooltipText.c_str());
    }
    delete td;
    return FALSE;
}

void GuiItem::onDragEnd(GtkWidget * widget, GdkDragContext * context, GtkSelectionData * sd, guint info, guint time, void * data)
{
    CHECK_IS_HANDLER_BLOCKED_NORET();

    GuiItem * it = (GuiItem*) data;
    string fname=sutl::format("file:%s", it->rec.fname.c_str());
    char * uris[2] =
    {   (char*) fname.c_str(), NULL};
    gtk_selection_data_set_uris(sd, uris);
}

gboolean GuiItem::onButton(GtkWidget *widget, GdkEventButton *e, void *data)
{
    CHECK_IS_HANDLER_BLOCKED();

    GuiItem * it = (GuiItem*) data;
    auto run = false;
    auto ret = false;
    if (e->type == GDK_BUTTON_PRESS)
    {
        if (e->button == 3 && !app->isBlocked())
        {
            app->setBlocked(true);
            app->properties->openForm(it->rec);
            app->setBlocked(false);
            ret = true;
        }
        if(!it->dragIconAdded && it->isApp() && it->rec.fname!="")
        {
            //deferred creation of the drag icon
            auto pixbuf = app->loader->getCachedPixbuf(it->rec.iconName, it->iconSize);
            if (pixbuf != NULL)
            {
                gtk_drag_source_set_icon_pixbuf(it->button, pixbuf);
            }
            it->dragIconAdded=true;
        }

    } else if (e->type == GDK_BUTTON_RELEASE)
    {
        if (e->button == 1 && it->mouseIn) //not dragging
        {
            run = true;
        }

    } else if (e->type == GDK_KEY_PRESS)
    {
        auto ke = (GdkEventKey*) e;
        auto key = ke->keyval;
        run = (key == GDK_Return || key == GDK_space);
        if (!run )
        {
            if(key > 32 && key < 128)
            {
                auto ent = it->guiwnd->searchBox->getEntry();
                gtk_widget_grab_focus((GtkWidget*)ent);
                string text = string(gtk_entry_get_text(ent));
                text += (char) key;
                gtk_entry_set_text( ent, text.c_str());
                gtk_editable_set_position((GtkEditable*) ent, 1000);
            }
            if (key == GDK_Right || key == GDK_Left)
            {
                auto otherList=it->isApp()?it->guiwnd->catList:it->guiwnd->appList;
                auto next=otherList->getSelectedItem();
                if(next!=NULL)
                {
                    gtk_widget_grab_focus(next->buttonWidget());
                }
            }

        }
    }
    if (run)
    {
        app->runMenuProcess(it);
        //ret = true;
    }
    return ret;
}

gboolean GuiItem::onMouseEnter(GtkWidget *widget, GdkEventButton *event, GtkWidget *data)
{
    CHECK_IS_HANDLER_BLOCKED();

    GuiItem * it = (GuiItem*) data;
    it->mouseIn = true;
    it->list->setItemState(it, GTK_STATE_PRELIGHT, true);
    if (it->isCategory())
    {
        it->list->changeCounter++;
        TimerRec * td = new TimerRec();
        td->item = it;
        td->count = it->list->changeCounter;
        g_timeout_add(CFGI("menu_sync_delay_ms"), onTimer, td);
    } else
    {
        it->list->setCurrent(it,true);
    }

    //Tooltips improvement. Longer delay and disaperence on MouseLeave
    if (!it->tooltipText.empty())
    {
        TimerRec * td = new TimerRec();
        td->item = it;
        td->type = td->TOOLTIP;
        g_timeout_add(app->tooltipDelayMs, onTimer, td);
    }

    return TRUE;
}

gboolean GuiItem::onMouseLeave(GtkWidget *widget, GdkEventButton *event, GtkWidget *data)
{
    CHECK_IS_HANDLER_BLOCKED();

    GuiItem * it = (GuiItem*) data;
    it->mouseIn = false;
    it->list->changeCounter++;
    it->list->setItemState(it, GTK_STATE_PRELIGHT, false);
    gtk_widget_set_tooltip_markup(it->button, NULL);
    return FALSE;
}

gboolean GuiItem::onFocus(GtkWidget *widget, GdkEvent *event, void*data)
{
    CHECK_IS_HANDLER_BLOCKED();
    GuiItem * it = (GuiItem*) data;
    it->list->setCurrent(it);
    return FALSE;
}

bool GuiItem::isApp()
{
    assert(list!=NULL);
    return list->isApp();
}

bool GuiItem::isCategory()
{
    return !isApp();
}

int GuiItem::isInCategory(GuiItem * category)
{
    if (category == guiwnd->categoryRecent && recentSerial)
        return true;
    if (category == guiwnd->categoryAll && rec.itemtype != res.itemPlace)
        return true;
    if (category == guiwnd->categoryFavorites)
    {
        string inFav = app->properties->get(this->rec, res.pkeyInFavorites);
        return inFav == res.parYes;
    }

    return this->category == category;
}

string GuiItem::searchText()
{
    string searchtext = rec.title;
    searchtext += " " + rec.loctitle;
    searchtext += " " + rec.comment;
    searchtext += " " + rec.loccomment;
    searchtext += " " + rec.genname;
    searchtext += " " + rec.locgenname;
    searchtext += " " + rec.command;
    searchtext += " " + rec.fname;
    return sutl::lower(searchtext);
}

string GuiItem::sortText(SortMode mode)
{

    if (mode == BY_RECENT)
    {
        return sutl::format("%8i %p", 1000000 - this->recentSerial, this);
    }

    string title = rec.loctitle != "" ? rec.loctitle : rec.title;
    string comment = rec.loccomment != "" ? rec.loccomment : rec.comment;
    if (title.length() == 0)
        title = rec.fname;

    string cmp = sutl::lower(title + " " + comment);
    return sutl::format("%8i%s %p", this->sort, cmp.c_str(), this);
}

void GuiItem::setTooltip()
{
    string tooltip;
    if ((CFGBOOL("app_enable_tooltips") && isApp()) || (CFGBOOL("category_enable_tooltips") && !isApp()))
    {
        string br = "\n";
        string labTitle = locTitle();
        string labComment = locComment();
        string genname = rec.locgenname == "" ? rec.genname : rec.locgenname;
        string loctitle = rec.loctitle == "" ? rec.title : rec.loctitle;

        tooltip = "<b>" + labTitle + "</b>";

        if (labTitle != genname && genname != "")
            tooltip = tooltip + br + genname;
        if (labTitle != loctitle && loctitle != "")
            tooltip = tooltip + br + loctitle;
        if (labComment != "")
            tooltip = tooltip + br + "<i>" + labComment + "</i>";

        tooltip = tooltip + br;

        string f1 = "<span font-size='small'>";
        string f2 = "</span>";
#define dispattr(lab,txt) tooltip=tooltip + br + f1 +"<b>" + (lab) + ":</b> " + (txt) + f2

        if (rec.command != "")
            dispattr(_("Command"), rec.command);
        if (rec.fname != "")
            dispattr((isApp() ? _("Launcher") : _("Directory file")), rec.fname);
        if (rec.iconName != "")
            dispattr(_("Icon"), rec.iconName);
        if (category != NULL)
            dispattr(_("Category"), category->rec.title);

        tooltip = sutl::prepareMarkup(tooltip);
        tooltipText = tooltip;
    }

}

string GuiItem::locTitle()
{
    return rec.loctitle.empty() ? rec.title : rec.loctitle;
}

string GuiItem::locComment()
{
    return rec.loccomment.empty() ? rec.comment : rec.loccomment;
}
