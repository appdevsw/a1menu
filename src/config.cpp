#include "config.h"
#include "configmap.h"
#include "guiparametersform.h"
#include "toolkit.h"
#include "sutl.h"
#include "application.h"
#include <assert.h>

using namespace std;

Config::Config()
{
}

Config::~Config()
{
    for (auto it : vitems)
        delete it;
}

void Config::load()
{
    for (auto it : vitems)
        delete it;
    vitems.clear();
    if (!Toolkit().fileExists(app->configFile))
    {
        setDefaultConfig();
        cmap.save(app->configFile);
    } else
    {
        setDefaultConfig();
        cmap.load(app->configFile);
    }
}

void Config::save()
{
    ConfigMap csave;
    for (auto it : vitems)
    {
        if (!it->isCategory)
            csave[it->name] = it->value;
    }
    csave.save(app->configFile);
}

ConfigMap & Config::mapref()
{
    return cmap;
}

ConfigItem * Config::newItem(ConfigItem * parentit, string name, int type, string value, string label, string lov, bool mandatory)
{
    static int idcounter = 2; //>1
    assert(parentit != NULL || vitems.size() == 0);
    ConfigItem * it = new ConfigItem();

    it->id = idcounter++;
    it->parent = parentit;
    it->name = name;
    it->labelorg = label;
    it->type = type;
    it->value = value;
    it->mandatory = mandatory;
    if (lov == "hotkey")
    {
        it->hotkey = true;
    } else if (sutl::contains(lov, ";"))
    {
        sutl::tokenize(lov, it->validItems, ";");
        it->mandatory = true;
    } else if (sutl::contains(lov, ":"))
    {
        sutl::tokenize(lov, it->validItems, ":");
        it->rangeMin = it->validItems[0];
        it->rangeMax = it->validItems[1];
        it->validItems.clear();
    }

    if (it->labelorg == "")
    {
        it->labelorg = it->name;
        it->isCategory = true;
    }

    string ltxt = it->labelorg;
    auto spl = sutl::split(ltxt, '|');
    if (spl.size() == 2)
        ltxt = sutl::trim(spl[1]);

    it->label = ltxt;
    vitems.push_back(it);
    return it;
}

void Config::setDefaultConfig()
{
    assert(vitems.size() == 0);

    int CHR = 0;
    int INT = res.parIntType;
    string YES = res.parYes;
    string NO = res.parNo;

    string listIconSizes = "48;32;24;22;16";
    string listBboolean = YES + ";" + NO;
    string lovRowHeight = "0:500";
    string listMenuLocation = res.locationAuto + ";" + res.locationManual;
    string listButtonPadding = res.labDefault + ";0;1;2;3;4;5;6;7;8;9";
    string listToolbarLocation = res.locationTop + ";" + res.locationBottom;

    auto root = newItem(0, _("Parameters"));

    auto c = newItem(root, _("Menu"));
    c->ord = 100;

    newItem(c, "menu_location", CHR, res.locationAuto, _("Window location"), listMenuLocation);
    newItem(c, "menu_sync_delay_ms", INT, "200", _("Synchronization delay [ms]"), "0:3000");
    newItem(c, "menu_label", CHR, "Menu", _("Menu label on the panel"), "", false);
    newItem(c, "icon_menu", CHR, "system-run", _("Menu icon on the panel"), "", false);
    auto h = newItem(c, "hotkey", CHR, "", _("Hot key"), "hotkey");
    h->ord += 1;
    h = newItem(c, "button_clear_cache", CHR, "", _("Clear cache"));
    h->buttonId=1;
    h->ord += 2;

    auto cl = root;

    c = newItem(cl, _("Applications"));
    c->ord = 110;

    string labvpad = _("Vertical padding");

    bool gtk3 = false;
#ifdef GTK3
    gtk3=true;
#endif

    newItem(c, "app_item_height", INT, "", _("Row height"), lovRowHeight);
    h = newItem(c, "app_item_padding", CHR, res.labDefault, labvpad, listButtonPadding, true);
    h->hidden = !gtk3;
    newItem(c, "app_icon_size", INT, "32", _("Icon size"), listIconSizes, true);
    newItem(c, "app_item_show_comment", CHR, YES, _("Show comments"), listBboolean);
    newItem(c, "app_item_show_text", CHR, YES, _("Show texts"), listBboolean);
    newItem(c, "app_item_show_icon", CHR, YES, _("Show icons"), listBboolean);
    newItem(c, "app_enable_tooltips", CHR, YES, _("Show tooltips"), listBboolean);
    newItem(c, "use_generic_names", CHR, NO, _("Use generic names"), listBboolean);
    newItem(c, "recent_entries_no", INT, "10", _("Max.number of recent entries"), "1:100");

    c = newItem(cl, _("Categories"));
    c->ord = 120;

    newItem(c, "category_item_height", INT, "", _("Row height"), lovRowHeight);
    h = newItem(c, "category_item_padding", CHR, res.labDefault, labvpad, listButtonPadding, true);
    h->hidden = !gtk3;
    newItem(c, "category_icon_size", INT, "24", _("Icon size"), listIconSizes, true);
    newItem(c, "category_item_show_text", CHR, YES, _("Show texts"), listBboolean);
    newItem(c, "category_item_show_icon", CHR, YES, _("Show icons"), listBboolean);
    newItem(c, "category_enable_tooltips", CHR, YES, _("Show tooltips"), listBboolean);
    newItem(c, "categories_on_the_right", CHR, NO, _("Display on the right"), listBboolean);

    auto cp = newItem(c, _("icons"));
    c->ord = 130;

    string clab = string(_("Category")) + " ";

    newItem(cp, "icon_category_all", CHR, "gtk-select-all", clab + _("`All`"), "", true);
    newItem(cp, "icon_category_favorites", CHR, "emblem-favorite", clab + _("`Favorites`"), "", true);
    newItem(cp, "icon_category_other", CHR, "applications-other", clab + _("`Other`"), "", true);
    newItem(cp, "icon_category_recent", CHR, "document-open-recent", clab + _("`Recently used`"), "", true);
    newItem(cp, "icon_category_places", CHR, "folder", clab + _("`Places`"), "", true);

    c = newItem(cl, _("Places"));
    c->ord = 140;

    newItem(c, "place_item_height", INT, "", _("Row height"), lovRowHeight);
    h = newItem(c, "place_item_padding", CHR, res.labDefault, labvpad, listButtonPadding, true);
    h->hidden = !gtk3;
    newItem(c, "place_icon_size", INT, "24", _("Icon size"), listIconSizes, true);
    newItem(c, "command_file_manager", CHR, "caja", _("File manager"));

    cp = newItem(c, _("commands"));

    newItem(cp, "command_place_computer", CHR, "$filemanager$ computer:", _("Computer"), "", true);
    newItem(cp, "command_place_home", CHR, "$filemanager$ ~", _("Home"), "", true);
    newItem(cp, "command_place_network", CHR, "$filemanager$ network:", _("Network"), "", true);
    newItem(cp, "command_place_trash", CHR, "$filemanager$ trash:", _("Trash"), "", true);
    newItem(cp, "command_place_desktop", CHR, "$filemanager$ $desktop$", _("Desktop"), "", true);

    cp = newItem(c, _("icons"));

    newItem(cp, "icon_place_computer", CHR, "computer", _("Computer"), "", true);
    newItem(cp, "icon_place_home", CHR, "folder_home", _("Home"), "", true);
    newItem(cp, "icon_place_network", CHR, "gtk-network", _("Network"), "", true);
    newItem(cp, "icon_place_trash", CHR, "emptytrash", _("Trash"), "", true);
    newItem(cp, "icon_place_desktop", CHR, "desktop", _("Desktop"), "", true);

    c = newItem(root, _("Toolbar"));
    c->ord = 150;
    newItem(c, "toolbar_icon_size", INT, "22", _("Icon size"), listIconSizes, true);
    newItem(c, "toolbar_location", CHR, res.locationBottom, _("Location"), listToolbarLocation, true);

    cp = newItem(c, _("commands"));
    newItem(cp, "command_shutdown", CHR, "mate-session-save --shutdown-dialog", _("Shutown"));
    newItem(cp, "command_lockscreen", CHR, "mate-screensaver-command -l", _("Lock screen"));
    newItem(cp, "command_logout", CHR, "mate-session-save --logout-dialog", _("Session logout"));

    cp = newItem(c, _("icons"));

    newItem(cp, "icon_toolbar_settings", CHR, "gnome-settings", _("Preferences"), "", true);
    newItem(cp, "icon_toolbar_reload", CHR, "gtk-refresh", _("Reload menu"), "", true);
    newItem(cp, "icon_toolbar_shutdown", CHR, "system-shutdown", _("Shutdown"), "", true);
    newItem(cp, "icon_toolbar_lockscreen", CHR, "system-lock-screen", _("Lock screen"), "", true);
    newItem(cp, "icon_toolbar_logout", CHR, "system-log-out", _("Logout"), "", true);

    for (auto it : vitems)
    {
        if (!it->isCategory)
            cmap[it->name] = it->value;
    }
}

vector<ConfigItem *> * Config::items()
{
    return &vitems;
}

