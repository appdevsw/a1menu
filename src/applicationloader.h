#ifndef MENUDIR_H
#define MENUDIR_H

#include <vector>
#include <set>
#include <map>

#include "desktoprec.h"


class ConfigMap;
class GuiItem;
class GuiList;

class ApplicationLoader
{
public:
    struct Place
    {
        std::string name;
        std::string cfgpar;
        std::string command;
        std::string icon;
        int sort;
        bool bookmark;
    };

    std::vector<string> vPaths;

	ApplicationLoader();
	virtual ~ApplicationLoader();
	void getItems(std::vector<DesktopRec> & arr);
	static std::string getMapLocValue(ConfigMap& cmap,const std::string& name);
	void insertPlaces(std::vector<DesktopRec>& itemarr);
	static void loadMap(const string& fname, ConfigMap & cmap);

private:
	bool parseFileEntry(ConfigMap& cmap, std::vector<DesktopRec> & arr,const std::string& fname);
};

#endif // MENUDIR_H
