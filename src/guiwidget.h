#ifndef GUIWIDGET_H_
#define GUIWIDGET_H_

#include <gtk/gtk.h>

class GuiWindow;

class GuiWidget
{
public:
    GuiWidget()
    {
    }

    virtual ~GuiWidget()
    {
    }

    virtual GtkWidget * widget()=0; //abstract

};

#endif /* GUIWIDGET_H_ */
