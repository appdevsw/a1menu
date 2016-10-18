#ifndef GUITOOLBAR_H_
#define GUITOOLBAR_H_

#include "guiwidget.h"
#include <string>

class GuiWindow;

class GuiToolbar: public GuiWidget
{
public:
    GuiToolbar();
    virtual ~GuiToolbar();
    void create(GuiWindow * wnd);
    GtkWidget * widget();
private:
    GtkWidget * toolbar = NULL;
    GtkWidget * alignment = NULL;
    GtkWidget * btnShutdown = NULL;
    GtkWidget * btnSettings = NULL;
    GtkWidget * btnLockScreen = NULL;
    GtkWidget * btnLogout = NULL;
    GtkWidget * btnReload = NULL;
    GuiWindow * guiwnd = NULL;

    void addButton(GtkWidget ** button, const std::string& iconName, const std::string& label);
    static gboolean onButtonClicked(GtkWidget *widget, void *data);
};

#endif /* GUITOOLBAR_H_ */
