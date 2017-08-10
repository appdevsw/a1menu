#ifndef APPLICATION_H_
#define APPLICATION_H_

#ifdef GTK3
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#include "guilist.h"
#include "guiwindow.h"
#include "guitoolbar.h"
#include "configmap.h"
#include "inotifymonitor.h"
#include "loader.h"
#include "enums.h"
#include <string>
#include <vector>
#include <set>
#include <map>
#include <thread>
#include <mutex>
#include <glib/gi18n.h>

class X11KeySequence;
class GuiWindow;
class Application;
class Config;
class ConfigMap;
class Properties;
class X11KeyListener;

//global configuration class with public accessible variables;

extern GtkWidget * getAppletBox();

class Application
{
public:
    static const char * A1MENU_GTK;
    static const char * A1MENU_GTK_RUN;
    static const char * A1MENU_VERSION;
    static const char * A1MENU_APPLET_NAME;
    static const char * A1MENU_APPLET_FACTORY;
    static const char * A1MENU_APPLET_SCHEMA;

    static const int SIG_RELOAD = SIGUSR1;
    static const int SIG_CLEAR_CACHE = SIGUSR2;
    static const int monitorFrequencyMs = 3000;

    struct TimerRec
    {
        int next()
        {
            static int serialcnt = 0;
            return ++serialcnt;
        }
        TimerAction action;
        bool clearCache = true;
        bool doShow = true;
        Loader * loader = NULL;
        std::thread * thr = NULL;
        int serial = next();
    };

    struct res_
    {
        // `~` to make unique
        string categoryAll = "All~";
        string categoryAll_loc = _("All");
        string categoryPlaces = "Places~";
        string categoryPlaces_loc = _("Places");
        string categoryRecent = "Recently used~";
        string categoryRecent_loc = _("Recently used");
        string categoryFavorites = "Favorites~";
        string categoryFavorites_loc = _("Favorites");
        string categoryDesktop = "Desktop~";
        string categoryDesktop_loc = _("Desktop");
        string categoryOther = "Other";
        string categoryOther_loc = _("Other");

        string toolShutdown = _("Shudown");
        string toolLockScreen = _("Lock screen");
        string toolLogout = _("Logout");
        string toolReload = _("Reload menu");
        string toolSettings = _("Menu preferences");

        string attrBookmark = "bookmark";

        string itemApp = "A";
        string itemCategory = "C";
        string itemPlace = "P";
        string itemIcon = "I";

        string pkeyCategoryAlias = "c";
        string pkeyInFavorites = "i";
        string pkeyDefaultCategory = "d";
        string pkeyMoveToCategory = "m";

        string labInFavorites = _("In favorites");
        string labProperties = _("Properties");
        string labDefaultCategory = _("Default category");
        string labMoveToCategory = _("Move to category");
        string labSearch = _("Search:");
        string labReloading = _("Reloading...");

        string labCatAlias = _("Category alias");

        string iconSearchBoxName = "search";
        int iconSearchBoxSize = 16;

        int parIntType = 2;

        //translated
        string parYes;
        string parNo;
        string locationAuto;
        string locationManual;
        string locationTop;
        string locationBottom;
        string labDefault;

    } res;

    string CFG(string key);
    int CFGI(string key);
    bool CFGBOOL(string key);

    Application();
    virtual ~Application();
    void init(int argc, char ** argv);
    static string menuDir();
    void start(bool inApplet = false);
    void quit();
    void startGui();
    void reload(bool clearCache = true);
    void onPressAppletButton();
    void setBlocked(bool enable);
    bool isBlocked();
    bool isReloading(string desc);
    GuiWindow * currentWindow();
    static void kbdListenerCallback(const X11KeySequence& kd);
    void runMenuProcess(GuiItem * it, string command = "");
    static void processEvents(int durationMs = 50);
    int getTimerToken();
    bool isTimerTokenValid(int token);
    void onNewAppletInstance();

    int argc = -1;
    char **argv = NULL;

    Config * config = NULL;
    Properties * properties = NULL;
    X11KeyListener * keyListener = NULL;
    string dirCache;
    string dirCacheIcons;
    string dirProperties;
    string dirTranslation;
    string loadFile;
    string configFile;
    string placementFile;
    int menuIconSize = 16;
    int tooltipDelayMs = 300; //will be added to the system tooltip delay

    Loader * loader = NULL;
    std::map<string, string> mapTranslations;

private:
    bool blocked = false;
    bool reloading = false;
    GuiWindow * guiwnd = NULL;
    INotifyMonitor inotifyMonitor;

    static gboolean onTimer(gpointer data);
    //void reloadProc(bool clearCache, bool doShow);
    void loaderThreadProc(TimerRec * td, bool clearCache, bool doShow);
    void startLoaderThread(bool clearCache, bool doShow);
    void finishLoaderThread(TimerRec * td);
    int timerToken = 0;
    int timerTokenMark = 0;
    std::mutex loaderMutex;
};

extern Application * app;

#define CFG(x) app->CFG(x)
#define CFGI(x) app->CFGI(x)
#define CFGBOOL(x) app->CFGBOOL(x)
#define res (app->res)

#define CHECK_IS_HANDLER_BLOCKED_NORET() if(app->isReloading(string(__FILE__)+": "+string(__FUNCTION__))) return
#define CHECK_IS_HANDLER_BLOCKED() CHECK_IS_HANDLER_BLOCKED_NORET() true
#define IFDELETE(obj) if(obj!=NULL) delete obj;obj=NULL;

#define APP_TEXT_DOMAIN "/usr/share/locale"

#endif /* APPLICATION_H_ */
