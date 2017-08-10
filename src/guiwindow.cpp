#include "guiwindow.h"
#include "guiitem.h"
#include "application.h"
#include "sutl.h"
#include "application.h"
#include "properties.h"
#include "toolkit.h"
#include <vector>
#include <cstring>
#include <assert.h>
#include "x11util.h"
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms-compat.h>
#include "guisearchbox.h"
#include "applet.h"

using namespace std;

GuiWindow::GuiWindow()
{
}

GuiWindow::~GuiWindow()
{
    if (widget() != NULL)
        gtk_widget_destroy(widget());
    if (catList != NULL)
        delete catList;
    if (appList != NULL)
        delete appList;
    if (toolbar != NULL)
        delete toolbar;
    if (searchBox != NULL)
        delete searchBox;

}
GtkWidget * GuiWindow::widget()
{
    return wnd;
}

void GuiWindow::create()
{
    assert(wnd==NULL);
    wnd = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_decorated(GTK_WINDOW(wnd), FALSE);
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(wnd), TRUE);
    gtk_widget_set_events(wnd, GDK_POINTER_MOTION_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
    gtk_window_set_type_hint(GTK_WINDOW(wnd), GDK_WINDOW_TYPE_HINT_MENU); //disable maximizing

    //constant elements that are not removed during reloading (rest of children are created and removed in populate())
    frame = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(wnd), frame);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_OUT);

    auto alignment = gtk_alignment_new(0.0, 0.0, 1.0, 1.0);
    gtk_alignment_set_padding((GtkAlignment *) alignment, 5, 0, 5, 5);
    gtk_container_add(GTK_CONTAINER(frame), alignment);

#ifdef GTK3
    {

        GtkStyle *style = gtk_widget_get_style(wnd);
        auto c = style->bg[GTK_STATE_NORMAL];
        string buf = sutl::format(" * {border-style: outset ;border-width: 1px; border-color: #%02x%02x%02x;}", //
                c.red >> 8, c.green >> 8, c.blue >> 8);
        auto prov = gtk_css_provider_new();
        gtk_css_provider_load_from_data(prov, buf.c_str(), -1, NULL);
        auto context = gtk_widget_get_style_context((GtkWidget*) wnd);
        gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(prov), GTK_STYLE_PROVIDER_PRIORITY_USER);
        g_object_unref(prov);

    }
#endif

    mainContainer = alignment;

#ifndef GTK3
    g_signal_connect(GTK_WIDGET(wnd), "size-request", G_CALLBACK(onStateChanged), this);
#endif
    g_signal_connect(GTK_WIDGET(wnd), "configure-event", G_CALLBACK(onStateChanged), this);
    g_signal_connect(GTK_WIDGET(wnd), "focus-out-event", G_CALLBACK(onFocus), this);
    g_signal_connect(GTK_WIDGET(wnd), "motion-notify-event", G_CALLBACK(onMouseEvent), this);
    g_signal_connect(GTK_WIDGET(wnd), "button-press-event", G_CALLBACK(onMouseEvent), this);
    g_signal_connect(GTK_WIDGET(wnd), "button-release-event", G_CALLBACK(onMouseEvent), this);
    g_signal_connect(GTK_WIDGET(wnd), "key-press-event", G_CALLBACK(onKeyEvent), this);
    g_signal_connect(GTK_WIDGET(wnd), "show", G_CALLBACK(onShow), this);

    populate();

}

void GuiWindow::populate(bool doShow)
{
    resizeInit = false;
    if (catList != NULL)
    {
        assert(mainChild!=NULL);
        gtk_container_remove(GTK_CONTAINER(mainContainer), (GtkWidget*) mainChild);
        delete catList;
        delete appList;
        delete toolbar;
        delete searchBox;

    }

    if (Applet::isRunning())
    {
        string menuIconPath = app->loader->getCachedIconPath(CFG("icon_menu"), app->menuIconSize);
        Applet::setButton(CFG("menu_label"), menuIconPath);
    }

    auto vbox = gtk_vbox_new(false, 0);
    mainChild = vbox;
    gtk_container_add(GTK_CONTAINER(mainContainer), vbox);
    hpane = gtk_hpaned_new();
    searchBox = new SearchBox();
    toolbar = new GuiToolbar();
    searchBox->create(this);
    toolbar->create(this);

    int margin = 3;
    if (CFG("toolbar_location") == res.locationTop)
    {
        gtk_box_pack_start((GtkBox*) vbox, toolbar->widget(), 0, 0, margin);
        gtk_box_pack_start((GtkBox*) vbox, gtk_hseparator_new(), 0, 0, 2);
        gtk_box_pack_start((GtkBox*) vbox, hpane, 1, 1, margin);
        gtk_box_pack_start((GtkBox*) vbox, gtk_hseparator_new(), 0, 0, 2);
    } else
    {
        gtk_box_pack_start((GtkBox*) vbox, hpane, 1, 1, margin);
        gtk_box_pack_start((GtkBox*) vbox, gtk_hseparator_new(), 0, 0, 2);
        gtk_box_pack_start((GtkBox*) vbox, toolbar->widget(), 0, 0, margin);
    }

    catList = new GuiList();
    appList = new GuiList();
    catList->create(this);
    appList->create(this);
    appList->makeScrollable();

    catBox = gtk_hbox_new(false, 0);
    auto sepcat = gtk_vseparator_new();

    //darker categories
    {
        auto w = gtk_bin_get_child((GtkBin*) catList->widget());
#ifdef GTK3
        auto wsrc = wnd;
        auto context = gtk_widget_get_style_context((GtkWidget*) wsrc);
        GdkRGBA c;
        gtk_style_context_get_background_color(context, GTK_STATE_FLAG_NORMAL, &c);
        double m = 0.95;
        c.blue *= m;
        c.red *= m;
        c.green *= m;
        gtk_widget_override_background_color(w, GTK_STATE_FLAG_NORMAL, &c);
#else
        GtkStyle *style = gtk_rc_get_style(w);
        auto c = style->bg[GTK_STATE_NORMAL];
        double m = 0.95;
        c.blue *= m;
        c.red *= m;
        c.green *= m;
        gtk_widget_modify_bg(GTK_WIDGET(w), GTK_STATE_NORMAL, &c);
#endif

    }

    if (CFGBOOL("categories_on_the_right"))
    {
        gtk_box_pack_start((GtkBox*) catBox, sepcat, 0, 0, 0);
        gtk_box_pack_start((GtkBox*) catBox, catList->widget(), 1, 1, 0);
        gtk_container_add(GTK_CONTAINER(hpane), appList->widget());
        gtk_container_add(GTK_CONTAINER(hpane), catBox);

    } else
    {
        gtk_box_pack_start((GtkBox*) catBox, catList->widget(), 1, 1, 0);
        gtk_box_pack_start((GtkBox*) catBox, sepcat, 0, 0, 0);
        gtk_container_add(GTK_CONTAINER(hpane), catBox);
        gtk_container_add(GTK_CONTAINER(hpane), appList->widget());
    }

#define addCategory(c,cloc,iconpar,psort) \
    c = new GuiItem(catList); \
    c->rec.title = res. c; \
    c->rec.loctitle = res. cloc; \
    c->rec.iconName = CFG(iconpar); \
    c->rec.itemtype = res.itemCategory; \
    c->sort = GuiItem::SORT_DEFAULT+psort; \
    c->create(this); \
    catList->add(c);

    addCategory(categoryAll, categoryAll_loc, "icon_category_all", -10);
    addCategory(categoryFavorites, categoryFavorites_loc, "icon_category_favorites", -9);
    addCategory(categoryRecent, categoryRecent_loc, "icon_category_recent", -8);
    addCategory(categoryPlaces, categoryPlaces_loc, "icon_category_places", -7);

#undef addCategory

    for (auto deskrec : app->loader->vapplications)
    {
        GuiItem * iapp = new GuiItem(appList);
        iapp->rec = deskrec;
        auto catrec = app->loader->vcategories[deskrec.category];
        //printf("\nFor item %s category %s -> %s",deskrec.title.c_str(),deskrec.category.c_str(),catrec.category.c_str());
        GuiItem * icat = NULL;
        if (!catrec.category.empty())
        {
            for (auto ic : *catList->items())
            {
                if (ic->rec.title.compare(catrec.category) == 0)
                {
                    icat = ic;
                    break;
                }
            }
            if (icat == NULL)
            {
                if (catrec.category == res.categoryOther)
                {
                    if (catrec.iconName.empty())
                        catrec.iconName = CFG("icon_category_other");
                    if (catrec.loctitle.empty())
                        catrec.loctitle = res.categoryOther_loc;
                }
                icat = new GuiItem(catList);
                icat->rec.title = catrec.category;
                icat->rec.comment = catrec.comment;
                icat->rec.iconName = catrec.iconName;
                icat->rec.loctitle = catrec.loctitle;
                icat->rec.loccomment = catrec.loccomment;
                icat->rec.itemtype = res.itemCategory;
                icat->rec.fname = catrec.dirFilePath;
                string alias = app->properties->get(icat->rec, res.pkeyCategoryAlias);
                if (!alias.empty())
                    icat->rec.loctitle = alias;
                icat->create(this);
                if (catrec.category == res.categoryDesktop)
                {
                    icat->sort = GuiItem::SORT_DEFAULT - 6;
                    categoryDesktop = icat;
                }
                catList->add(icat);
            }
            iapp->category = icat;
        } else
        {
            iapp->category = categoryOther;
        }
        if (iapp->rec.iconName.empty() && icat != NULL)
            iapp->rec.iconName = icat->rec.iconName;
        iapp->create(this);
        if (!iapp->rec.attr[res.attrBookmark].empty())
            iapp->sort++;
        appList->add(iapp);
    }

    int space = 3;
    if (categoryDesktop != NULL)
        categoryDesktop->addSpace(space);
    else if (categoryPlaces != NULL)
        categoryPlaces->addSpace(space);

    appList->sort(DEFAULT);
    catList->sort(DEFAULT);

    if (doShow && //
            (!Applet::isRunning() || app->isReloading("populate")))
    {
        auto isv = gtk_widget_get_visible(wnd);
        show();
        if (isv)
            goToDefaultCategory();
    }

}

int GuiWindow::xwinID()
{

#ifdef GTK3
    int xwinid = gdk_x11_window_get_xid(gtk_widget_get_window(GTK_WIDGET(wnd)));
#else
    int xwinid = GDK_WINDOW_XWINDOW(GTK_WIDGET (widget())->window);
#endif
    assert(xwinid > 0);
    return xwinid;
}

gboolean GuiWindow::onShow(GtkWidget *widget, void*data)
{
    CHECK_IS_HANDLER_BLOCKED();

    GuiWindow * wnd = (GuiWindow*) data;
    wnd->searchBox->setText("");
    wnd->goToDefaultCategory();
    return FALSE;
}

void GuiWindow::show(bool enable)
{
    if (enable)
    {
        gtk_widget_show_all(widget());
        loadPlacement();
        setDefaultSize();
        gtk_window_set_skip_taskbar_hint(GTK_WINDOW(wnd), TRUE);
        app->processEvents();
        X11Util::setForegroundWindow(xwinID());

        //transparecny after show()
        if (gtk_widget_is_composited(wnd))
        {
            double opacity = 1.0 - ((double) CFGI("transparency")) / 100.0;
#ifdef GTK3
            gtk_widget_set_opacity(wnd, opacity);
#endif
            gtk_window_set_opacity((GtkWindow*) wnd, opacity);
        }

    } else
        gtk_widget_hide(widget());
}

void GuiWindow::setDefaultSize()
{
    bool isConfig = Toolkit().fileExists(app->placementFile);
    if (!isConfig)
    {
        //catList has a disabled scorll viewport, so we can obtain a full size
        GtkAllocation alloc;
        gtk_widget_get_allocation(catList->widget(), &alloc);
        gtk_paned_set_position((GtkPaned*) hpane, alloc.width + 3);

        Placement p;
        gtk_window_get_size(GTK_WINDOW(widget()), &p.w, &p.h);
        int hmax = gdk_screen_height();
        if (p.h > hmax)
            p.h = hmax;
        gtk_window_resize((GtkWindow*) widget(), alloc.width * 2 + 150, p.h);
    }
    catList->makeScrollable();
    app->processEvents();
    resizeInit = true;
    if (!isConfig)
        savePlacement();
}

gboolean GuiWindow::onStateChanged(GtkWidget *widget, GdkEvent *event, void*data)
{
    CHECK_IS_HANDLER_BLOCKED();

    GuiWindow * wnd = (GuiWindow*) data;
    wnd->savePlacement();
    return FALSE;
}

gboolean GuiWindow::onFocus(GtkWidget *widget, GdkEvent *event, void*data)
{
    CHECK_IS_HANDLER_BLOCKED();

    GuiWindow * wnd = (GuiWindow*) (data);
    if (!app->isBlocked())
    {
        gtk_widget_hide(wnd->widget());
    }
    return FALSE;
}

gboolean GuiWindow::onKeyEvent(GtkWidget *widget, GdkEvent *event, void*data)
{
    CHECK_IS_HANDLER_BLOCKED();

    GuiWindow * wnd = (GuiWindow*) (data);
    GdkEventKey* e = (GdkEventKey*) event;
    if (e->keyval == GDK_Escape)
    {
        if (wnd->searchBox->getText().empty())
        wnd->show(false);
        else
        wnd->searchBox->setText("");
        return TRUE;
    }
    if (!gtk_widget_is_focus((GtkWidget*) wnd->searchBox->getEntry()) //
            && e->keyval == GDK_BackSpace//
            && !wnd->searchBox->getText().empty())
    {
        string t = wnd->searchBox->getText();
        t = t.substr(0, t.length() - 1);
        wnd->searchBox->setText(t);
        gtk_widget_grab_focus((GtkWidget*) wnd->searchBox->getEntry());
        gtk_editable_select_region((GtkEditable *) wnd->searchBox->getEntry(), 2000, 2000);
        return TRUE;
    }
    return FALSE;
}

void GuiWindow::getEdge(int x, int y, int& edge, int& cursor)
{
    int m = resizeMargin;
    int width, height;
    gtk_window_get_size(GTK_WINDOW(widget()), &width, &height);

    edge = -1;
    cursor = GDK_ARROW;
    if ((y <= m || y >= height - m) && x > m && x < width - m)
    {
        edge = GDK_WINDOW_EDGE_NORTH;
        cursor = GDK_HAND1;

    } else if (y <= m && x <= m)
    {
        edge = GDK_WINDOW_EDGE_NORTH_WEST;
        cursor = GDK_TOP_LEFT_CORNER;
    } else if (y <= m && x >= width - m)
    {
        edge = GDK_WINDOW_EDGE_NORTH_EAST;
        cursor = GDK_TOP_RIGHT_CORNER;
    } else if (y >= height - m && x <= m)
    {
        edge = GDK_WINDOW_EDGE_SOUTH_WEST;
        cursor = GDK_BOTTOM_LEFT_CORNER;
    } else if (y >= height - m && x >= width - m)
    {
        edge = GDK_WINDOW_EDGE_SOUTH_EAST;
        cursor = GDK_BOTTOM_RIGHT_CORNER;
    }
    if (edge < 0 && x <= m)
    {
        edge = GDK_WINDOW_EDGE_WEST;
        cursor = GDK_LEFT_SIDE;
    }
    if (edge < 0 && x >= width - m)
    {
        edge = GDK_WINDOW_EDGE_EAST;
        cursor = GDK_RIGHT_SIDE;
    }

}

gboolean GuiWindow::onMouseEvent(GtkWidget *widget, GdkEvent *event, void*data)
{
    CHECK_IS_HANDLER_BLOCKED();

    GuiWindow * wnd = (GuiWindow*) (data);

    if (event->type == GDK_MOTION_NOTIFY)
    {
        static map<int, GdkCursor *> curmap;
        GdkEventMotion* e = (GdkEventMotion*) event;
        int edge, cursor;
        wnd->getEdge(e->x, e->y, edge, cursor);
        if (cursor != GDK_TOP_RIGHT_CORNER && cursor != GDK_BOTTOM_RIGHT_CORNER)
        {
            cursor = GDK_ARROW;
        }
        if (curmap.find(cursor) == curmap.end())
        {
            curmap[cursor] = gdk_cursor_new((GdkCursorType) cursor);
        }
        gdk_window_set_cursor(gtk_widget_get_window(wnd->widget()), curmap[cursor]);
    }

    if (event->type == GDK_BUTTON_PRESS)
    {
        GdkEventButton* e = (GdkEventButton*) event;
        int width, height, edge, cursor;
        gtk_window_get_size(GTK_WINDOW(wnd->widget()), &width, &height);
        wnd->getEdge(e->x, e->y, edge, cursor);
        if (edge == GDK_WINDOW_EDGE_NORTH)
        {
            gtk_window_begin_move_drag(GTK_WINDOW(wnd->widget()), e->button, e->x_root, e->y_root, e->time);
            return TRUE;
        } else if (edge >= 0)
        {
            gtk_window_begin_resize_drag(GTK_WINDOW(wnd->widget()), (GdkWindowEdge) edge, e->button, e->x_root, e->y_root, e->time);
            return TRUE;
        }
    }
    if (event->type == GDK_BUTTON_RELEASE)
    {
        wnd->savePlacement();
    }

    return FALSE;
}

void GuiWindow::goToDefaultCategory()
{
    int idxDefault = -1;
    int idxAll = -1;
    int count = 0;
    for (auto it : *catList->items())
    {
        if (app->properties->getbool(it->rec, res.pkeyDefaultCategory))
        {
            idxDefault = count;
            break;
        }
        if (it->rec.title == res.categoryAll)
        {
            idxAll = count;
        }
        count++;
    }
    if (idxDefault < 0)
        idxDefault = idxAll;
    if (idxDefault >= 0)
    {
        auto it = (*catList->items())[idxDefault];
        catList->setCurrent(it, true);
    }

}

void GuiWindow::loadPlacement()
{
    gtk_window_set_gravity((GtkWindow*) wnd, GDK_GRAVITY_STATIC);
    GtkWidget *appletBox = Applet::getAppletBox();
    bool positionAuto = appletBox != NULL && CFG("menu_location") == res.locationAuto;
    ConfigMap cmap;
    cmap.load(app->placementFile);
    if (cmap.geti("w") > 0 && cmap.geti("h") > 0)
    {
        //printf("\nplacement restore x%i y%i w%i h%i split%i\n"//
        //   , cmap.geti("x"), cmap.geti("y"), cmap.geti("w"), cmap.geti("h"), cmap.geti("splitx"));
        gtk_window_resize((GtkWindow*) wnd, cmap.geti("w"), cmap.geti("h"));
        if (!positionAuto)
            gtk_window_move((GtkWindow*) wnd, cmap.geti("x"), cmap.geti("y"));
        if (cmap.geti("splitx") > 10)
            gtk_paned_set_position((GtkPaned*) hpane, cmap.geti("splitx"));
    }

    if (positionAuto)
    {
        gint wx, wy;
        gtk_window_set_position((GtkWindow*) wnd, GTK_WIN_POS_NONE);
        gdk_window_get_origin(gtk_widget_get_window(appletBox), &wx, &wy);
        gtk_window_set_gravity((GtkWindow*) wnd, GDK_GRAVITY_SOUTH_WEST);
        gtk_window_move((GtkWindow*) wnd, wx, wy);
        X11Util::setPosition(xwinID(), wx, wy);
        app->processEvents();
    }
}

void GuiWindow::savePlacement()
{
    if (!resizeInit)
        return;
    Placement p;
    gtk_window_get_position(GTK_WINDOW(wnd), &p.x, &p.y);
    gtk_window_get_size(GTK_WINDOW(wnd), &p.w, &p.h);
    p.splitx = gtk_paned_get_position((GtkPaned*) hpane);
    if (!memcmp(&p, &prevPlacement, sizeof(Placement)))
    {
        return;
    }

    GdkWindow *gwin = gtk_widget_get_window(GTK_WIDGET(wnd));
    if (gwin == NULL)
        return;
    if (p.w > 0 && p.h > 0 && gtk_widget_get_visible(wnd))
    {
        ConfigMap cmap;
        cmap.puti("x", p.x);
        cmap.puti("y", p.y);
        cmap.puti("w", p.w);
        cmap.puti("h", p.h);
        cmap.puti("splitx", p.splitx);
        cmap.save(app->placementFile);
        //printf("\nplacement save x%i y%i w%i h%i split%i\n"//
        //, cmap.geti("x"), cmap.geti("y"), cmap.geti("w"), cmap.geti("h"),cmap.geti("splitx"));
        prevPlacement = p;
    }
}

