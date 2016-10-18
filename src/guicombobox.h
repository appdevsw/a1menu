#ifndef A1MENU_GTK_SRC_GUICOMBOBOX_H_
#define A1MENU_GTK_SRC_GUICOMBOBOX_H_

#include "guiwidget.h"
#include <string>
#include <map>
#include <vector>

using std::string;

class GuiComboBox: public GuiWidget
{
public:

    GuiComboBox();
    virtual ~GuiComboBox();
    GtkWidget * widget();
    void add(string,string);
    string get();
    void set(string);
private:
    GtkWidget * combo=NULL;
    std::vector<string> vkeys;
};

#endif /* A1MENU_GTK_SRC_GUICOMBOBOX_H_ */
