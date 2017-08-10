#ifndef A1MENU_GTK_SRC_APPLET_H_
#define A1MENU_GTK_SRC_APPLET_H_

#include <mate-panel-applet.h>
#include <mate-panel-applet-gsettings.h>
#include <string>
#include <map>

class Applet
{
public:
    Applet(MatePanelApplet *mateApplet);
    virtual ~Applet();

    static gboolean appletFactory(MatePanelApplet * mateApplet, const char* iid, gpointer data);
    static void setButton(const std::string& label, const std::string& iconPath);
    static GtkWidget * getAppletBox();
    static int appletMain(int argc, char *argv[]);
    static bool isRunning();

private:

    static std::map<MatePanelApplet*, Applet*> vInstances;
    static Applet * lastInstance;

    MatePanelApplet * mateApplet = NULL;
    GtkWidget *labelMem = NULL;
    GtkWidget *frameMem = NULL;
    GtkWidget *hbox = NULL;
    GtkWidget *hboxi = NULL;
    GtkWidget *icon = NULL;

    void init();
    static void appletDestroy(MatePanelApplet *mateApplet, Applet * appinst);
    static gboolean onButtonPress(GtkWidget *widget, GdkEventButton *event, Applet * appinst);
    static gboolean onMouseInOut(GtkWidget *widget, GdkEventButton *event, Applet * appinst);
};

#endif /* A1MENU_GTK_SRC_APPLET_H_ */
