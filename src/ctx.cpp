#include <ctx.h>
#include <stdlib.h>
#include <assert.h>
#include <QMessageBox>
#include "toolkit.h"
#include "categoryloader.h"
#include "loader.h"
#include "parameterform.h"
#include "configmap.h"
#include "x11util.h"
#include <QDir>

//global functions and variables

namespace ctx
{
MainWindow * wnd = NULL;
QApplication * application = NULL;
ItemList * appList = NULL;
ItemList * catList = NULL;
SearchBox * searchBox = NULL;
ConfigMap& cfgorg = *(new ConfigMap());
ConfigMap& cfgmod = *(new ConfigMap());
X11Util& x11Util = *(new X11Util());
X11KeyListener &keyListener = *(new X11KeyListener());
QString localeName=QLocale().name();
//QString localeName="en_GB";


//IconLoader& iconLoader = *(new IconLoader());
//CategoryLoader& categoryLoader = *(new CategoryLoader());
Loader * loader = NULL;
QPalette palorg, palorglist;
bool standaloneMode = true;
thread_data_t thread_data;

const QString procParRunStandalone = "--run";
const QString procParImgCache = "--iconcache";
const QString procParConfig = "--config";
const QString procParLoader = "--loader";
const QString procParResource = "--crres";
const QString procParHideCtrlButton = "--hidebutton";

Item * categoryAll = NULL;
Item * categoryFavorites = NULL;
Item * categoryRecent = NULL;
Item * categoryOther = NULL;
Item * categoryPlaces = NULL;

QString nameCategoryAll;
QString nameCategoryFavorites;
QString nameCategoryRecent;
QString nameCategoryOther;
QString nameCategoryPlaces;
QString nameCategoryDesktop;

QMenu *menu = NULL;
QString menuProperties;
QString menuAddToDesktop;
QString menuAddToFavorites;
QString menuRemoveFromFavorites;
QString menuSetDefaultCategory;

ToolButton * buttonShutdown = NULL;
ToolButton * buttonLock = NULL;
ToolButton * buttonLogout = NULL;
ToolButton * buttonSettings = NULL;
ToolButton * buttonReload = NULL;

std::atomic<int> isReloading(0);

std::vector<places_item_t> places;

clockmicro_t clockmicro()
{
	return Toolkit().GetMicrosecondsTime();
}

void errorDialog(QString err)
{
	QMessageBox::critical(0, "Menu error", err);
}

QString CFG(QString key)
{
	return cfgmod[key];
}

bool CFGBOOL(QString key)
{
	return cfgmod[key] == ParameterForm::YES;
}

ConfigMap * CFGPTR()
{
	return &cfgmod;
}

//function not working! but macro works
/*
 const char * QS(const QString& qs)
 {
 return qs.toStdString().c_str();
 }
 */

QString path(Path path)
{
	QString name = "a1menu";
	QString base = QDir::homePath() + "/." + name + "/";
	switch (path)
	{
	case BASE:
		return base;
	case ITEM_PROPERTIES:
		return base + "items/";
	case GLOBAL_PROPERTIES:
		return base + "prop.conf";
	case TRANSLATIONS:
		return base + "ts/";
	case CACHE:
		return base + "cache/";
	case CSS_DEFAULT:
		return base + name + "-default.css";
	case CFG_FILE:
		return base + name + ".conf";
	case LOCATION:
		return base + "location.conf";
	case TMP_DEBPKG:
		return "/tmp/" + name + "/";
	case XDG_ICON_PATH:
		return "/usr/share/icons";
	case DLG_ICONS_MAP:
		return base + "cache/dlgicons.conf";
	default:
	{
		fprintf(stderr, "cfgPath error\n");
		exit(1);
	}
	}
	return "";
}

}

