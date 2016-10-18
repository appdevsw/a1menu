#include "applicationloader.h"
#include "desktoprec.h"
#include "guiitem.h"
#include "configmap.h"
#include "config.h"
#include "toolkit.h"
#include "configmap.h"
#include "sutl.h"
#include <dirent.h>
#include <sstream>
#include <fstream>
#include <cstdio>
#include <assert.h>
#include <set>
#include <gtk/gtk.h>

using namespace std;

ApplicationLoader::ApplicationLoader()
{
    Toolkit t;
    string dir = t.homeDir() + "/.local/share/applications/";
    if (t.dirExists(dir))
        vPaths.push_back(dir);
    vPaths.push_back("/usr/share/applications/");
    dir = t.desktopDir();
    if (!dir.empty() && t.dirExists(dir))
        vPaths.push_back(dir);

}

ApplicationLoader::~ApplicationLoader()
{
}

string ApplicationLoader::getMapLocValue(ConfigMap& cmap, const string& name)
{
    static string ln = std::locale("").name();
    string name2 = name + "[" + ln.substr(0, 2) + "]";
    string val = cmap[name2];
    if (val != "")
        return val;
    string name5 = name + "[" + ln.substr(0, 5) + "]";
    val = cmap[name5];
    if (val != "")
        return val;

    string udomain = cmap["X-Ubuntu-Gettext-Domain"];
    string src = cmap[name];
    if (!udomain.empty() && src != "")
    {
        string tdom = string(textdomain(NULL));
        textdomain(udomain.c_str());
        val = gettext(src.c_str());
        textdomain(tdom.c_str());
        if (val != "" && val != src)
        {
            return val;
        }
    }
    return "";
}

bool ApplicationLoader::parseFileEntry(ConfigMap& tmap, std::vector<DesktopRec> & arr, const string& fname)
{
    Toolkit toolkit;

    //printf("\n%s", fname.c_str());

    auto desk = tmap["onlyshowin"];
    if (!desk.empty() && !sutl::contains(desk, "MATE"))
        return false;
    if (sutl::contains(tmap["notshowin"], "MATE"))
        return false;
    if (sutl::lower(tmap["nodisplay"]) == "true" || tmap["exec"] == "")
        return false;

    vector<string> vcategories;
    sutl::tokenize(tmap["categories"], vcategories, ";");
    if (vcategories.size() == 1 && vcategories[0] == "Screensaver")
        return false;

    DesktopRec rec;

    string icon = tmap["icon"];
    string locicon = getMapLocValue(tmap, "icon");
    if (locicon != "")
    {
        if (icon == "" || sutl::contains(icon, "launcher"))
            icon = locicon;
        if (icon != "" && !sutl::contains(icon, "launcher") && !sutl::contains(locicon, "launcher"))
            icon = locicon;
    }
    rec.iconName = icon;
    rec.title = tmap["name"];
    rec.genname = tmap["genericname"];
    rec.command = tmap["exec"];
    rec.comment = tmap["comment"];
    rec.categories = tmap["categories"];
    rec.vcategories = vcategories;
    rec.vcatset.insert(vcategories.begin(), vcategories.end());
    rec.fname = fname;
    rec.loctitle = getMapLocValue(tmap, "name");
    rec.locgenname = getMapLocValue(tmap, "genericname");
    rec.loccomment = getMapLocValue(tmap, "comment");
    arr.push_back(rec);
    return true;
}

void ApplicationLoader::loadMap(const string& fname, ConfigMap & cmap)
{
    Toolkit toolkit;
    string ln = std::locale("").name();
    string loc1 = "[" + ln.substr(0, 2) + "]";
    string loc2 = "[" + ln.substr(0, 5) + "]";

    ifstream infile(fname);
    string line;
    bool isentry = false;
    while (getline(infile, line))
    {
        line = sutl::trim(line);
        if (sutl::startsWith(line, "["))
        {
            if (isentry)
                break;
            isentry = sutl::contains(line, "[Desktop Entry]");
            continue;
        }

        if (!isentry)
            continue;

        bool skip = sutl::startsWith(line, "X-") || sutl::startsWith(line, "MimeType");
        if (sutl::contains(line, "Gettext-Domain"))
            skip = false;
        if (!skip)
        {
            auto pos = line.find("=");
            if (pos != string::npos)
            {
                string key = sutl::trim(line.substr(0, pos));
                int p = sutl::indexOf(key, "[");
                if (p < 0 || sutl::contains(key, loc1) || sutl::contains(key, loc2))
                {
                    string val = sutl::trim(line.substr(pos + 1));
                    cmap[key] = val;
                }
            }
        }

    }

}

void ApplicationLoader::getItems(vector<DesktopRec> & arr)
{
    vector<string> files;
    Toolkit t;

    for (string path : vPaths)
    {
        if (t.dirExists(path))
            t.getDirectory(path, &files, ".desktop");
    }
    for (string fname : files)
    {
        if (!sutl::endsWith(fname, ".desktop"))
            continue;
        ConfigMap cmap;
        loadMap(fname, cmap);
        parseFileEntry(cmap, arr, fname);

    }

}

void ApplicationLoader::insertPlaces(vector<DesktopRec>& itemarr)
{

    std::vector<Place> cplaces = { { _("Computer"), "place_computer" } //
            , { _("Home folder"), "place_home" } //
            , { _("Network"), "place_network" } //
            , { _("Desktop"), "place_desktop" } //
            , { _("Trash"), "place_trash" } //
    };

    std::vector<Place> vplaces = cplaces;
    for (auto place : vplaces)
        place.sort = GuiItem::SORT_DEFAULT;

    Toolkit toolkit;
    string fbookmarks = toolkit.homeDir() + "/.gtk-bookmarks";
    if (toolkit.fileExists(fbookmarks))
    {
        vector<string> v;
        toolkit.getFileLines(fbookmarks, v);
        for (auto line : v)
        {
            Place p;
            string command, name;
            int psp = sutl::indexOf(line, " ");
            if (psp >= 0)
            {
                command = line.substr(0, psp);
                name = line.substr(psp + 1);
            } else
            {
                command = line;
                name = command;
            }
            name = sutl::replace(name, "file:///", "");
            p.name = name;
            p.command = "$filemanager$ \"" + command + "\"";
            p.icon = CFG("icon_category_places");
            p.bookmark = true;
            vplaces.push_back(p);
        }
    }

    for (auto place : vplaces)
    {
        DesktopRec rec;
        rec.title = place.name;
        if (place.command == "")
            rec.command = CFG("command_" + place.cfgpar);
        else
            rec.command = place.command;
        if (place.cfgpar != "")
            rec.iconName = CFG("icon_" + place.cfgpar);
        rec.category = res.categoryPlaces;

        rec.vcategories=
        {   "Places"};
        if (place.bookmark)
            rec.attr[res.attrBookmark] = res.parYes;
        itemarr.push_back(rec);
    }
}

