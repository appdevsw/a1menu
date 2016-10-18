#ifndef A1MENU_GTK_SRC_GUIPARAMETERSFORM_H_
#define A1MENU_GTK_SRC_GUIPARAMETERSFORM_H_

#include "guiwidget.h"
#include "config.h"
#include "guihotkeyfield.h"
#include "guicombobox.h"

class PFormItem;

class GuiParametersForm: public GuiWidget
{
    friend class PFormItem;
public:

    GuiParametersForm();
    virtual ~GuiParametersForm();
    int create(int parentPID);
    GtkWidget * widget();
    static int run();
private:
    static volatile bool isThreadRunning;
    int parentPID = 0;
    GtkWidget * pwnd = NULL;
    GtkWidget * table = NULL;
    Config config;
    vector<PFormItem *> vitems;

    void buildGuiTree(ConfigItem * parent, int level = 0);
    int validateForm();
    static void threadProc();

};

class PFormItem
{
public:
    PFormItem(GuiParametersForm * pform);
    ~PFormItem();

    int setHidden(int enable);
    bool isCollapsed();
    int level();
    int isChildOf(int id);
    void createGui();
    static gboolean onButtonClicked(GtkWidget *widget, void *data);

    //printf("\nvalidation result %i\n", vres);

    ConfigItem * configItem = NULL;

    GtkWidget * labelBox = NULL;
    GtkWidget * fieldbox = NULL;
    GtkWidget * field = NULL;
    GtkWidget * entry = NULL;
    GtkWidget * button = NULL;
    GuiComboBox * guicombo = NULL;
    GtkWidget * hotkeygtk = NULL;
    GuiHotKeyField * hotkeyobj = NULL;

    GuiParametersForm * pform;
    PFormItem * parent = NULL;
    bool collapsed = false;


};

#endif /* A1MENU_GTK_SRC_GUIPARAMETERSFORM_H_ */
