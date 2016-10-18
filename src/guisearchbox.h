#ifndef SEARCHBOX_H_
#define SEARCHBOX_H_

#include "guiwidget.h"
#include <string>

class GuiWindow;

class SearchBox: public GuiWidget
{
public:
    SearchBox();
    virtual ~SearchBox();
    void create(GuiWindow * wnd);
    GtkWidget * widget();
    GtkEntry * getEntry();
    void setText(std::string txt);
    std::string getText();
private:
    GtkWidget * box = NULL;
    GtkWidget * entry = NULL;
    static gboolean onSearchBoxChanged(GtkWidget *widget, GdkEvent *event, void *data);
    static gboolean onKey(GtkWidget *widget, GdkEvent *event, void *data);
    std::string prevSearchText = "";
    GuiWindow * guiwnd = NULL;

};

#endif /* SEARCHBOX_H_ */
