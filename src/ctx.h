#ifndef QT_MENU_CTX_H_
#define QT_MENU_CTX_H_

//#include <x11util.h>
#include <QLineEdit>
#include <QMenu>
#include <QApplication>
#include <QString>
#include <QMutex>
#include <vector>
#include <string>
#include <atomic>
//#include "configmap.h"
//#include "config.h"
//#include "iconloader.h"
//#include "categoryloader.h"
//#include "itemlist.h"
//#include "mainwindow.h"
//#include "searchbox.h"

#define QS(qstring) (qstring).toStdString().c_str()
#define PATH(x) ctx::path(ctx::Path::x)

#define TIMERINIT() auto t1=ctx::clockmicro()
#define TIMER(desc,xt1,xt2) auto xt2=ctx::clockmicro();qDebug("%s %10lld",desc,xt2-xt1)

class CategoryLoader;
class IconLoader;
class Loader;
class MainWindow;
class ItemList;
class Item;
class SearchBox;
class ConfigMap;
class X11Util;
class X11KeyListener;
class ToolButton;

namespace ctx
{

enum Path
{
	BASE, ITEM_PROPERTIES, TRANSLATIONS, CACHE, CSS_DEFAULT, CFG_FILE,//
	LOCATION, TMP_DEBPKG, XDG_ICON_PATH, GLOBAL_PROPERTIES,DLG_ICONS_MAP
};

extern QString path(Path path = Path::BASE);

extern MainWindow * wnd;
extern QApplication * application;
extern ItemList * appList;
extern ItemList * catList;
extern SearchBox * searchBox;
extern ConfigMap &cfgorg, &cfgmod;
extern X11Util &x11Util;
extern X11KeyListener &keyListener;

extern const QString procParRunStandalone;
extern const QString procParImgCache;
extern const QString procParConfig;
extern const QString procParLoader;
extern const QString procParResource;
extern const QString procParHideCtrlButton;

extern const QString debpkgTmpPath;
extern const QString ipcKeyFile;
extern QString localeName;

//extern IconLoader& iconLoader;
//extern CategoryLoader& categoryLoader;
extern Loader * loader;
extern QPalette palorg, palorglist;
extern bool standaloneMode;

extern Item * categoryAll;
extern Item * categoryFavorites;
extern Item * categoryRecent;
extern Item * categoryOther;
extern Item * categoryPlaces;

extern QString nameCategoryAll;
extern QString nameCategoryFavorites;
extern QString nameCategoryRecent;
extern QString nameCategoryOther;
extern QString nameCategoryPlaces;
extern QString nameCategoryDesktop;

extern QMenu *menu;
extern QString menuProperties;
extern QString menuAddToDesktop;
extern QString menuAddToFavorites;
extern QString menuRemoveFromFavorites;
extern QString menuSetDefaultCategory;

extern ToolButton * buttonShutdown;
extern ToolButton * buttonLock;
extern ToolButton * buttonLogout;
extern ToolButton * buttonSettings;
extern ToolButton * buttonReload;

extern std::atomic<int> isReloading;

struct places_item_t
{
	QString name;
	QString par;
	QString command;
	QString icon;
	int sort;
};
extern std::vector<places_item_t> places;
typedef long long int clockmicro_t;
extern clockmicro_t clockmicro();

struct thread_data_t
{
	MainWindow * mainQtWindow;
	int argc;
	char ** argv;
	int inthread;
	int hideCtrlButton=0;
	int isGtkInitialized = 0;
	clockmicro_t tstart;
};
extern thread_data_t thread_data;


extern void errorDialog(QString err);

extern QString CFG(QString key);
extern bool CFGBOOL(QString key);
extern ConfigMap * CFGPTR();

}

#endif /* QT_MENU_CTX_H_ */
