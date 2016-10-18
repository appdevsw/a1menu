#ifndef A1MENU_GTK_SRC_LOADER_H_
#define A1MENU_GTK_SRC_LOADER_H_

#include <gtk/gtk.h>
#include <string>
#include <vector>
#include "categoryloader.h"
#include "desktoprec.h"

using std::string;

class Loader
{
public:


    Loader();
    virtual ~Loader();
    void runInProcess(int argc, char ** argv,bool clearCache=false);
    void loadProc();
    string getCachedIconPath(const string& iconName, int iconSize);
    GtkWidget * getCachedIcon(const string& iconName, int iconSize);
    GdkPixbuf * getCachedPixbuf(const string& iconName, int iconSize);

    std::vector<DesktopRec> vapplications;
    map<string, CategoryLoader::CategoryRec> vcategories;
    map<string, string> vicons;

private:
    GtkIconTheme * iconTheme = NULL;
    char attrEscape= '~';
    string createCachedIcon(const string& iconName, int iconSize);
    string iconKey(const string& iconName, int iconSize);
    string toCachedPath(const string& iconPathOrg, int iconSize);
    void clearCache();
    void clientLoad();

};

#endif /* A1MENU_GTK_SRC_LOADER_H_ */
