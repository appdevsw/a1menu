#include "applicationloader.h"
#include "item.h"
#include "iconloader.h"
#include "configmap.h"
#include "config.h"
#include "toolkit.h"
#include "ctx.h"
#include "configmap.h"
#include <QDomElement>
#include <QDomDocument>
#include <QString>
#include <QLocale>
#include <QTextStream>
#include <QIODevice>
#include <QFile>
#include <dirent.h>
#include <sstream>
#include <fstream>
#include <cstdio>
#include <assert.h>
#include <set>
#include <QDir>
#include <QFileInfo>
#include <QStringBuilder>

using namespace std;
using namespace ctx;

ApplicationLoader::ApplicationLoader()
{
	desktopPath = Config::getDesktopPath();
}

ApplicationLoader::~ApplicationLoader()
{
}

QString ApplicationLoader::getMapLocValue(ConfigMap& cmap, QString name)
{
	static QString ln = ctx::localeName;
	QString n1 = name % "[" % ln.mid(0, 2) % "]";
	QString val = cmap[n1];
	if (val != "")
		return val;
	QString n2 = name % "[" % ln.mid(0, 5) % "]";
	return cmap[n2];
}

Item * ApplicationLoader::parseFileEntry(ConfigMap& tmap, std::vector<Item*> & arr, QString& fname)
{
	Toolkit toolkit;

	auto desk = tmap["onlyshowin"];
	if (!desk.isEmpty() && !desk.contains("MATE;"))
	{
		if (desk.contains("KDE;") || desk.contains("XFCE;"))
		{
			//qDebug("desk %s %s", QS(desk), QS(fname));
			return NULL;
		}
	}

	if (tmap["nodisplay"].toLower() == "true" || tmap["exec"] == "")
	{
		return NULL;
	}

	vector<QString> vcat;
	toolkit.tokenize(tmap["categories"], vcat, ";");
	if (vcat.size() == 1 && vcat[0] == "Screensaver")
		return NULL;

	Item * item = new Item();
	item->load.type = Item::ITEM_TYPE_APP;

	QString icon = tmap["icon"];
	QString locicon = getMapLocValue(tmap, "icon");
	if (locicon != "")
	{
		if (icon == "" || icon.contains("launcher"))
			icon = locicon;
		if (icon != "" && !icon.contains("launcher") && !locicon.contains("launcher"))
			icon = locicon;
	}
	item->rec.iconName = icon;

	item->rec.title = tmap["name"];
	item->rec.genname = tmap["genericname"];
	item->rec.loctitle = getMapLocValue(tmap, "name");
	item->rec.locgenname = getMapLocValue(tmap, "genericname");
	item->rec.command = tmap["exec"];
	item->rec.comment = tmap["comment"];
	item->rec.loccomment = getMapLocValue(tmap, "comment");
	item->rec.vcategories = vcat;
	item->rec.fname = fname;

	if (!desktopPath.isEmpty() && fname.startsWith(desktopPath))
	{
		item->load.desktop = true;
	}
	arr.push_back(item);
	return item;
}

void ApplicationLoader::parseFile(QString& fname, vector<Item*> & arr)
{
	//TIMERINIT();

	Toolkit toolkit;
	int isentry = 0;
	ConfigMap cmap;

	FILE *f = fopen(QS(fname), "r");
	if (f == NULL)
	{
		perror(QS(fname));
		return;
	}

	QString ln = ctx::localeName;
	QString loc1 = "[" % ln.mid(0, 2) % "]";
	QString loc2 = "[" % ln.mid(0, 5) % "]";

	QTextStream in(f, QIODevice::ReadOnly);
	in.setCodec("UTF-8");
	QString line;
	bool eof = false;
	while (!eof)
	{
		if ((eof = in.atEnd()))
			line = "[";
		else
			line = in.readLine();
		if (line.trimmed().startsWith("["))
		{
			if (isentry)
			{
				Item * it = parseFileEntry(cmap, arr, fname);
				if (it != NULL)
					it->rec.fname = fname;
			}
			isentry = 0;
		}

		if (line.contains("[Desktop Entry]"))
		{
			isentry = 1;
			cmap.clear();
		}

		if (isentry //
		&& !line.startsWith("X-") //
				&& !line.startsWith("MimeType") //
						)
		{
			int pos = line.indexOf("=");
			if (pos >= 0)
			{
				QString key = line.mid(0, pos).trimmed();
				int p = key.indexOf("[");
				if (p < 0 || key.indexOf(loc1) >= 0 || key.indexOf(loc2) >= 0)
				{
					cmap[key] = line.mid(pos + 1).trimmed();
					//qDebug("%s",QS(line));
				}
			}
		}
	}
	fclose(f);

	//TIMER(QS(fname), t1, t2);

}

void ApplicationLoader::getItems(vector<Item*> & arr)
{
	vector<QString> paths;
	paths.push_back(QDir::homePath() + "/.local/share/applications/");
	std::vector<QString> adddirs;
	CFGPTR()->split("desktop_additional_paths", adddirs);
	for (QString apath : adddirs)
	{
		paths.push_back(apath);
	}
	paths.push_back("/usr/share/applications/");
	QString desktopPath = Config::getDesktopPath();
	if (!desktopPath.isEmpty())
		paths.push_back(desktopPath);
	vector<QString> files;
	Toolkit toolkit;

	for (QString path : paths)
	{
		toolkit.getDirectory(path, &files, ".desktop");
	}

	for (QString fname : files)
	{
		if (!QString(fname).endsWith(".desktop"))
			continue;
		parseFile(fname, arr);
	}

}

void ApplicationLoader::insertPlaces(vector<Item *>& itemarr)
{

	//int isize = CFG("place_icon_size").toInt();
	std::vector<ctx::places_item_t> vplaces = ctx::places;
	for (auto place : vplaces)
		place.sort = Item::Order::ord_default;

	QString fbookmarks = QDir::homePath() + "/.gtk-bookmarks";
	Toolkit toolkit;
	if (toolkit.fexists(fbookmarks))
	{
		vector<QString> v;
		toolkit.getFileLines(fbookmarks, v);
		for (auto line : v)
		{
			ctx::places_item_t p;
			QString command, name;
			int psp = line.indexOf(" ");
			if (psp >= 0)
			{
				command = line.mid(0, psp);
				name = line.mid(psp + 1);
			} else
			{
				command = line;
				name = command;
			}
			name.replace("file:///", "");
			p.name = name;
			p.command = "$filemanager$ \"" + command + "\"";
			p.icon = CFG("icon_category_places");
			p.sort = Item::Order::ord_place_gtk;
			vplaces.push_back(p);
		}
	}
	int type = Item::ITEM_TYPE_PLACE | Item::ITEM_TYPE_APP;

	for (auto place : vplaces)
	{
		Item *item = new Item(0, place.sort);
		item->load.type = type;
		item->rec.title = place.name;
		if (place.command == "")
			item->rec.command = CFG("command_" + place.par);
		else
			item->rec.command = place.command;
		if (place.par != "")
			item->rec.iconName = CFG("icon_" + place.par);

		item->rec.vcategories=
		{	"Places"};
		itemarr.push_back(item);
	}

	Item *isep = new Item(0, Item::Order::ord_default + 1, true);
	isep->load.type = type;
	isep->rec.vcategories=
	{	"Places"};

	itemarr.push_back(isep);

}
