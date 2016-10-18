#ifndef A1MENU_GTK_SRC_GUIHOTKEYFIELD_H_
#define A1MENU_GTK_SRC_GUIHOTKEYFIELD_H_

#include "guiwidget.h"
#include <map>

class GuiHotKeyField :public GuiWidget
{
public:
    GuiHotKeyField();
    virtual ~GuiHotKeyField();
    GtkWidget * widget();
    void create();
private:
    GtkWidget * entry=NULL;
    static gboolean onKey(GtkWidget *widget, GdkEvent *e, void *data);

};

#endif /* A1MENU_GTK_SRC_GUIHOTKEYFIELD_H_ */
