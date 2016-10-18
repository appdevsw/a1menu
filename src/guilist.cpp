#include "guilist.h"
#include <assert.h>
#include "guiitem.h"
#include "application.h"
#include "toolkit.h"
#include "sutl.h"
#include "application.h"

using namespace std;

GuiList::GuiList()
{
}
GuiList::~GuiList()
{
    for (auto it : vitems)
        delete it;
}

GtkWidget * GuiList::widget()
{
    return scroll;
}

void GuiList::create(GuiWindow * wnd)
{
    this->guiwnd = wnd;
    scroll = gtk_scrolled_window_new( NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_NEVER, GTK_POLICY_NEVER);
    table = gtk_table_new(0, 0, 0);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scroll), table);

    if (this == wnd->appList && CFGBOOL("categories_on_the_right"))
        gtk_scrolled_window_set_placement(GTK_SCROLLED_WINDOW(scroll), GTK_CORNER_TOP_RIGHT);

    gtk_container_set_border_width(GTK_CONTAINER(table), 2);
    gtk_viewport_set_shadow_type(GTK_VIEWPORT(gtk_bin_get_child((GtkBin* )scroll)), GTK_SHADOW_NONE);

}

void GuiList::add(GuiItem *it)
{
    it->list = this;
    vitems.push_back(it);
    GtkWidget * wrapper = gtk_hbox_new(true, 0);
    guint count;
    gtk_table_get_size((GtkTable*) table, &count, NULL);
    GtkAttachOptions optx = (GtkAttachOptions) (GTK_EXPAND | GTK_FILL);
    GtkAttachOptions opty = GTK_SHRINK;
    gtk_table_attach((GtkTable*) table, wrapper, 1, 2, count + 1, count + 2, optx, opty, 0, 0);
    gtk_container_add(GTK_CONTAINER(wrapper), it->widget());
}

std::vector<GuiItem *> * GuiList::items()
{
    return &vitems;
}

void GuiList::goNext(GuiItem * it)
{
    if (!it)
        it = getSelectedItem();
    int pos = -1;
    for (auto i : vitems)
    {
        if (i == it && pos == -1)
            pos = 0;
        else if (pos == 0 && gtk_widget_get_visible(i->widget()))
        {
            gtk_widget_grab_focus(i->buttonWidget());
            return;
        }

    }
}

void GuiList::makeScrollable()
{
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    for (auto it : vitems)
    {
        if (it->labTitle != NULL)
            gtk_label_set_ellipsize((GtkLabel*) it->labTitle, PANGO_ELLIPSIZE_END);
    }
    scrollable = true;
}

bool GuiList::isScrollable()
{
    return scrollable;
}

void GuiList::setCurrent(GuiItem * it)
{
    assert(it->list == this);
    setItemState(it, GTK_STATE_SELECTED, true);
    if (isCategory())
    {
        guiwnd->appList->refresh("");
    }
}

GuiItem * GuiList::getSelectedItem()
{
    for (auto it : vitems)
    {
        if (it->state.isSelected)
            return it;
    }
    if (isCategory())
        return guiwnd->categoryAll;
    return NULL;
}

bool GuiList::isApp()
{
    return this == guiwnd->appList;
}

bool GuiList::isCategory()
{
    return !isApp();
}

void GuiList::setItemState(GuiItem * itm)
{

    if (itm->state.isSelected)
    {
        itm->toButton();
    } else if (itm->state.isHiglight)
    {
        itm->toButton();
    } else
    {
        gtk_widget_set_state(itm->widget(), GTK_STATE_NORMAL);
        itm->toFlat();
    }

}

void GuiList::setItemState(GuiItem * it, int state, bool enable)
{
    set<GuiItem *> vnewSelected;
    for (auto itm : vselected)
    {
        if (state == GTK_STATE_SELECTED && itm->state.isSelected)
            itm->state.isSelected = 0;
        if (state == GTK_STATE_PRELIGHT && itm->state.isHiglight)
            itm->state.isHiglight = 0;
        if (state == GTK_STATE_NORMAL)
        {
            itm->state.isHiglight = 0;
            itm->state.isSelected = 0;
        }
        setItemState(itm);
        if (itm->state.isSelected || itm->state.isHiglight)
            vnewSelected.insert(itm);
    }
    vselected = vnewSelected;

    if (state == GTK_STATE_SELECTED)
        it->state.isSelected = enable;
    if (state == GTK_STATE_PRELIGHT)
        it->state.isHiglight = enable;
    if (state == GTK_STATE_NORMAL)
        it->state.isHiglight = 0, it->state.isHiglight = 0;

    setItemState(it);
    if (it->state.isSelected || it->state.isHiglight)
        vselected.insert(it);

}

void GuiList::sort(SortMode mode)
{
    if (lastSortMode == mode)
        return;
    Toolkit t;
    lastSortMode = mode;
    int swaps = 0;
    for (size_t i1 = 0; i1 < vitems.size(); i1++)
    {
        size_t ix = i1;
        string ixtext = vitems[ix]->sortText(mode);
        for (size_t i2 = i1 + 1; i2 < vitems.size(); i2++)
        {
            if (ixtext > vitems[i2]->sortText(mode))
            {
                ix = i2;
                ixtext = vitems[ix]->sortText(mode);
            }
        }

        if (mode == SortMode::BY_RECENT && vitems[ix]->recentSerial == 0)
            continue;

        if (ix != i1)
        {
            swaps++;
            auto it1 = vitems[i1]->widget();
            auto it2 = vitems[ix]->widget();
            GtkWidget *parent1 = gtk_widget_get_parent(it1);
            GtkWidget *parent2 = gtk_widget_get_parent(it2);

            g_object_ref(it1);
            g_object_ref(it2);
            gtk_widget_reparent(it1, parent2);
            gtk_widget_reparent(it2, parent1);
            g_object_unref(it1);
            g_object_unref(it2);

            GuiItem * swap = vitems[i1];
            vitems[i1] = vitems[ix];
            vitems[ix] = swap;
        }
    }
}

void GuiList::refresh(string filterText)
{
    Toolkit toolkit;
    vector<string> filter;
    if (!filterText.empty())
        sutl::tokenize(sutl::lower(filterText), filter);
    GuiItem * category = guiwnd->catList->getSelectedItem();
    if (category == NULL)
        category = guiwnd->categoryAll;

    bool useFilter = !filter.empty();
    if (useFilter)
        gtk_widget_hide(guiwnd->catBox);
    else
        gtk_widget_show(guiwnd->catBox);

    if (category == guiwnd->categoryRecent)
        sort(BY_RECENT);
    else
        sort(DEFAULT);

    int count = 0;
    for (auto it : vitems)
    {
        int show = true;
        if (useFilter)
        {
            string searchText = it->searchText();
            for (auto sf : filter)
                if (searchText.find(sf) == string::npos)
                {
                    show = false;
                    break;
                }
        } else if (it->isInCategory(category))
        {
            show = true;
        } else
        {
            show = false;
        }
        if (show)
        {
            it->show(true);
            if (count++ == 0)
                setCurrent(it);
        } else
            it->show(false);

    }
}

void GuiList::addToRecent(GuiItem * it)
{
    static int recentCounter = 0;
    it->recentSerial = ++recentCounter;
    map<int, GuiItem *> vrecent;
    for (auto vi : vitems)
    {
        if (vi->recentSerial)
            vrecent[vi->recentSerial] = vi;

    }
    int count = vrecent.size();
    int cmax = CFGI("recent_entries_no");
    for (auto e : vrecent)
    {
        if (count-- > cmax)
            e.second->recentSerial = 0;
    }
}

