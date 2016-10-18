#ifndef A1MENU_GTK_SRC_PROPERTIES_H_
#define A1MENU_GTK_SRC_PROPERTIES_H_

#include <string>
#include <map>
#include <gtk/gtk.h>
#include "configmap.h"

using std::string;

class DesktopRec;

class Properties
{
public:


    Properties();
    virtual ~Properties();
    string get(DesktopRec& rec, const string& key);
    bool getbool(DesktopRec& rec, const string& key);
    void set(DesktopRec& rec,const string& key, const string& val);
    void openForm(DesktopRec& rec);
private:
    std::map<string, ConfigMap*> cache;
    GtkWidget * table=NULL;

    string getHash(DesktopRec& rec);
    ConfigMap * getMap(DesktopRec& rec);
    void addRow(const string& label,GtkWidget * field,bool visible);
    string filePath(string hash);
};

#endif /* A1MENU_GTK_SRC_PROPERTIES_H_ */
