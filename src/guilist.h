#ifndef GUILIST_H_
#define GUILIST_H_

#include <gtk/gtk.h>
#include <stdio.h>
#include <vector>
#include <set>
#include <string>

#include "enums.h"
#include "guiwidget.h"

class GuiWindow;
class GuiItem;

class GuiList: public GuiWidget
{
public:

    std::vector<GuiItem *> * items();
    int changeCounter = -1;

    GuiList();
    virtual ~GuiList();
    void create(GuiWindow * wnd);
    void add(GuiItem * it);
    GtkWidget * widget();
    void refresh(std::string filter);
    void setCurrent(GuiItem * it);
    bool isApp();
    bool isCategory();
    void setItemState(GuiItem * it, int state, bool enable);
    void sort(SortMode mode);
    GuiItem * getSelectedItem();
    void addToRecent(GuiItem * it);
    void goNext(GuiItem * it = NULL);
    void makeScrollable();
    bool isScrollable();

private:
    bool scrollable = false;
    SortMode lastSortMode = UNDEFINED;
    std::set<GuiItem *> vselected;
    std::vector<GuiItem *> vitems;

    GuiItem * prevSelected = NULL;
    GuiWindow * guiwnd = NULL;
    GtkWidget * table = NULL;
    GtkWidget * scroll = NULL;

    void setItemState(GuiItem * it);

};

#endif /* GUILIST_H_ */
