#ifndef GUIITEM_H_
#define GUIITEM_H_
#include <gtk/gtk.h>
#include <vector>
#include "guiwidget.h"
#include "application.h"
#include <string>

#include "desktoprec.h"
#include "enums.h"

using std::string;
using std::vector;

class GuiList;

class GuiItem: public GuiWidget
{
    friend class GuiList;
    friend class GuiWindow;
    friend class Properties;

public:

    const static int SORT_DEFAULT = 100;

    DesktopRec rec;

    struct TimerRec
    {
        const static int SET_CURRENT_ROW = 0;
        const static int TOOLTIP = 1;
        int type = SET_CURRENT_ROW;
        int count;
        int timerToken = app->getTimerToken();
        GuiItem * item;
    };

    struct State
    {
        bool isSelected = 0;
        bool isHiglight = 0;
    } state;

    GuiItem(GuiList * list);
    virtual ~GuiItem();
    GtkWidget * widget();
    GtkWidget * buttonWidget();
    void create(GuiWindow * wnd);
    bool isApp();
    bool isCategory();
    void toButton();
    void toFlat();
    void show(bool enable);
    int isInCategory(GuiItem * category);
    string searchText();
    string sortText(SortMode mode);
    string locTitle();
    string locComment();
    void setTooltip();
    void addSpace(int y);

private:
    GuiWindow * guiwnd = NULL;
    int recentSerial = 0;
    bool mouseIn = false;
    GuiList * list = NULL;
    string tooltipText;
    int iconSize = 0;
    bool dragIconAdded = false;
    int serial = 0;
    GuiItem * category = NULL;
    int sort = SORT_DEFAULT;

    GtkWidget * button = NULL;
    GtkWidget * container = NULL;
    GtkWidget * icon = NULL;
    GtkWidget * labTitle = NULL;
    GtkWidget * labComment = NULL;

    void makeLabel(GtkWidget * label, bool bold, bool italic);
    void handleContextMenu(GdkEventButton * e);

    static gboolean onButton(GtkWidget *widget, GdkEventButton *event, void *data);
    static gboolean onMouseEnter(GtkWidget *widget, GdkEventButton *event, GtkWidget *data);
    static gboolean onMouseLeave(GtkWidget *widget, GdkEventButton *event, GtkWidget *data);
    static gboolean onFocus(GtkWidget *widget, GdkEvent *event, void*data);
    static gboolean onContextMenu(GtkWidget *widget, GdkEvent *event);
    static gboolean onTimer(gpointer data);

    static void onDragEnd(GtkWidget * widget, GdkDragContext * context, GtkSelectionData * selection_data, guint info, guint time,
            void * data);

};

#endif /* GUIITEM_H_ */
