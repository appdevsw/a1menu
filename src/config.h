#ifndef CONFIG_H
#define CONFIG_H

#include "configmap.h"
#include "application.h"
#include <vector>

using std::string;

struct ConfigItem
{
    int id;
    ConfigItem * parent=NULL;
    int type = 0;
    bool mandatory = false;
    string name;
    string value;
    string labelorg;
    string label;
    std::vector<string> validItems;
    string rangeMin ;
    string rangeMax ;
    bool isCategory = false;
    bool hotkey = false;
    int ord = 100;
    bool hidden=false;
    int buttonId=0;
};

class Config
{
public:
    explicit Config();
    virtual ~Config();
    void setDefaultConfig();
    vector<ConfigItem *> * items();
    ConfigMap & mapref();
    void load();
    void save();
private:
    ConfigMap cmap;
    vector<ConfigItem *> vitems;

    ConfigItem * newItem(ConfigItem * parentid, string name, int type = 0, string value = "", string label = "", string lov = "", //
            bool mandatory = false);

};

#endif // CONFIG_H
