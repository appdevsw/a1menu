#ifndef GTK_LNK_GUIWINDOW_H_
#define GTK_LNK_GUIWINDOW_H_

#include <gtk/gtk.h>
#include <stdio.h>

#include "guiwidget.h"
#include <string>

class GuiList;
class GuiToolbar;
class SearchBox;
class GuiItem;

class GuiWindow: public GuiWidget
{
public:
    GuiWindow();
    virtual ~GuiWindow();
    GtkWidget * widget();
    void create();
    void populate(bool doShow=true);
    void loadPlacement();
    void savePlacement();
    void show(bool enable=true);
    GtkWidget * wnd = NULL;
    GuiList *appList= NULL;
    GuiList *catList= NULL;
    GtkWidget * catBox=NULL;
    GuiToolbar *toolbar= NULL;
    SearchBox *searchBox= NULL;

    GuiItem * categoryOther= NULL;
    GuiItem * categoryAll= NULL;
    GuiItem * categoryPlaces= NULL;
    GuiItem * categoryRecent= NULL;
    GuiItem * categoryFavorites= NULL;
    GuiItem * categoryDesktop= NULL;

private:
    struct Placement
    {
        int x=-1;
        int y;
        int w;
        int h;
        int splitx;
    } prevPlacement;

    bool resizeInit=false;



    GtkWidget * hpane = NULL;
    GtkWidget * frame = NULL;
    GtkWidget * mainContainer = NULL;
    GtkWidget * mainChild = NULL;
    int xwinID();

    void goToDefaultCategory();
    void setDefaultSize();
    void getEdge(int x,int y,int& edge,int& cursor);

    static gboolean onStateChanged(GtkWidget *widget, GdkEvent *event, void*data);
    static gboolean onFocus(GtkWidget *widget, GdkEvent *event, void*data);
    static gboolean onMouseEvent(GtkWidget *widget, GdkEvent *event, void*data);
    static gboolean onKeyEvent(GtkWidget *widget, GdkEvent *event, void*data);
    static gboolean onShow(GtkWidget *widget, void*data);


    const static int resizeMargin = 8;

};

#endif /* GTK_LNK_GUIWINDOW_H_ */
