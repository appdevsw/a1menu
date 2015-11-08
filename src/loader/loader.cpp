#include "loader.h"
#include <QProcess>
#include <assert.h>
#include "ctx.h"
#include "configmap.h"
#include "iconloader.h"
#include "imgconv.h"
#include "categoryloader.h"
#include "applicationloader.h"
#include "itemproperties.h"
#include "config.h"
#include "searchbox.h"
#include "userevent.h"
#include "mainwindow.h"
#include "toolbutton.h"
#include "toolkit.h"
#include "itemlist.h"
#include "item.h"
#include "itemproperties.h"
#include <x11util.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <sstream>
#include <QTextStream>
#include <QStringBuilder>
#include <QThread>

using namespace ctx;
using std::string;

Loader::lcons Loader::str;

namespace key
{
QString TP_ITEM = "it";
QString TP_ICON = "ic";
QString TP_HIDDEN = "hidden";

QString KEY_TYPE = "t";
QString KEY_SUBTYPE = "st";
QString KEY_ITEMTYPE = "ittp";
QString KEY_SERIAL = "id";
QString KEY_DUPID = "dupid";

QString KEY_SYMBOL = "sy";
QString KEY_PATH = "p";
QString KEY_SIZE = "si";
QString KEY_THEME = "th";

QString KEY_DESKTOP = "ds";
QString KEY_SORT = "so";
QString KEY_ITEM_ICON = "itic";
}

using namespace key;

Loader::Loader()
{
}

Loader::~Loader()
{
	if (process != NULL)
		delete process;
	for (auto it : vitems)
		delete it;
	for (auto it : vcategories)
		delete it;
	//qDebug("loader destructor, pid %i", getpid());
}

#define useCache false

void Loader::initServer()
{
	assert(process==NULL);
	if (useCache)
	{
		auto ctlvalid = checkCtlFile();
		//qDebug("checkCtlFile %i",ctlvalid);
		if (ctlvalid)
			return;
	}

	clearLoaderFiles();
	process = new QProcess();
	QStringList args;
	args.append(ctx::procParLoader);
	process->setProcessChannelMode(QProcess::ForwardedChannels);
	process->start(ctx::thread_data.argv[0], args);
	process->waitForStarted();
	srvpid = process->pid();
}

void Loader::pack(ConfigMap& cmap, QString& msg)
{
	int count = 0;
	for (const auto e : cmap.getMapRef())
	{
		if (count++ > 0)
			msg.append("\n");
		msg.append(e.first);
		msg.append("=");
		msg.append(e.second.value);
	}
}

void Loader::unpack(const QString& msg, ConfigMap& cmap)
{
	for (auto line : msg.split("\n"))
	{
		size_t pos = line.indexOf("=");
		if (pos >= 0)
		{
			cmap[line.mid(0, pos).trimmed()] = line.mid(pos + 1).trimmed();
		}
	}
}

string Loader::encode(const QString& msg)
{
	std::stringstream buf;
	QByteArray ba = msg.toLocal8Bit();
	char spec[5];
	for (auto cc : ba)
	{
		int c = cc & 0xFF;
		if (c < 32 || c > 127 || c == '\\')
			buf << (sprintf(spec, "\\%03i", c), spec);
		else
			buf << cc;
	}
	return buf.str();
}
QString Loader::decode(const char * line)
{
	std::stringstream buf;
	const char * p = line;
	char c;
	while ((c = *(p++)))
	{
		if (c == '\\')
		{
			int code = (*(p) - '0') * 100 + (*(p + 1) - '0') * 10 + (*(p + 2) - '0');
			buf << (char) code;
			p += 3;
		} else
			buf << c;
	}
	return QString::fromLocal8Bit(buf.str().c_str(), buf.str().size());
}

void Loader::srvSend(std::ofstream& fout, ConfigMap& cmap)
{
	QString msg;
	pack(cmap, msg);
	fout << encode(msg) << "\n";
}

void Loader::srvRequestIcon(std::ofstream& fout, QString subtype, QString ptr, QString symbol, int size, QString theme)
{
	QString path = iconLoader.getIconPath(symbol, size, theme);
	if (!path.isEmpty())
	{

		QString cpath = iconLoader.getCachePath(path, size);
		ConfigMap cmap;
		cmap[KEY_SYMBOL] = symbol;
		cmap[KEY_SIZE] = QString::number(size);
		if (!theme.isEmpty())
			cmap[KEY_THEME] = theme;
		cmap[KEY_SUBTYPE] = subtype;
		cmap[KEY_SERIAL] = ptr;
		cmap[KEY_TYPE] = TP_ICON;
		cmap[KEY_PATH] = cpath;
		srvSend(fout, cmap);
	}
}

#define FOUT_INIT(xfname) std::ofstream fout;\
	fout.open(QS(xfname+".tmp"), std::ios::out | std::ios::binary);\
    assert(fout.is_open())

#define FOUT_CLOSE(xfname) fout.close();QFile(xfname+".tmp").rename(xfname);

void Loader::srvInitEnv()
{
	qDebug("srv init environment...");
	QString cfgPaths[] = { PATH(BASE), PATH(ITEM_PROPERTIES), PATH(CACHE), PATH(TRANSLATIONS) };
	for (QString path : cfgPaths)
	{
		QDir dir;
		if (!dir.mkpath(path))
		{
			perror(QS(path));
			qDebug("Cannot create directory %s", QS(path));
			exit(1);
		}
	}
	FOUT_INIT(str.fenv);

	ctx::application = new QApplication(ctx::thread_data.argc, ctx::thread_data.argv);
	iconLoader.setCachingInSeparateProcess(false);
	Config::installDefaultFiles();
	Config::initTranslation();
	Config config;
	QString cfgFile = PATH(CFG_FILE);
	config.setDefaultConfig(ctx::cfgorg);
	if (Toolkit().fexists(cfgFile))
		ctx::cfgorg.load(cfgFile);
	ctx::cfgorg.setCheckExistence(true);
	Config::inheritIncludes(ctx::cfgorg, ctx::cfgmod);
	ctx::cfgorg.save(cfgFile);

	FOUT_CLOSE(str.fenv);
	qDebug("srv init environment done.");
}

void Loader::srvLoadIcons()
{
	qDebug("srv load icons...");

	FOUT_INIT(str.ficons);

	srvRequestIcon(fout, "search1", "", "search", 16, "");
	srvRequestIcon(fout, "search2", "", "edit-clear", 16, "matefaenza");

	srvRequestIcon(fout, "dlg-reset", "", "gtk-refresh", 16, "");
	srvRequestIcon(fout, "dlg-cancel", "", "gtk-close", 16, "");
	srvRequestIcon(fout, "dlg-save", "", "gtk-save", 16, "");
	srvRequestIcon(fout, "dlg-apply", "", "gtk-apply", 16, "");

	{
		int bs = 24;
		QString psymbol, ptheme;
		QString bid, ptr;

#define breq(sym) bid=sym; \
		iconLoader.parseIconParameter(CFG(sym), psymbol, ptheme); \
		srvRequestIcon(fout,bid,"",psymbol,bs,ptheme)

		breq("icon_toolbar_shutdown");
		breq("icon_toolbar_lock");
		breq("icon_toolbar_logout");
		breq("icon_toolbar_settings");
		breq("icon_toolbar_reload");

		bs = CFG("category_icon_size").toInt();

		breq("icon_category_all");
		breq("icon_category_favorites");
		breq("icon_category_recent");
		breq("icon_category_places");
		breq("icon_category_other");

		bs = 16;
		breq("icon_menu");

		//bs = CFG("category_icon_size").toInt();
		//breq("icon_place_desktop");

	}

	for (auto it : vitems)
	{
		QString ptr = QString::number(it->load.serial);
		int isize = CFG("app_icon_size").toInt();
		bool isPlace = it->load.type & Item::ITEM_TYPE_PLACE;
		if (isPlace)
		{
			isize = CFG("place_icon_size").toInt();
		}

		QString customIcon = ItemProperties(it).get(ItemProperties::pn.IconPath);
		QString path;
		QString subtype = KEY_ITEM_ICON;
		QString symbol = customIcon;

		if (symbol.isEmpty())
			symbol = it->rec.iconName;

		if (symbol.isEmpty() && isPlace)
			symbol = CFG("icon_category_places");
		srvRequestIcon(fout, subtype, ptr, symbol, isize, "");
	}
	for (auto it : vcategories)
	{
		QString ptr = QString::number(it->load.serial);

		QString customIcon = ItemProperties(it).get(ItemProperties::pn.IconPath);
		QString cat = it->rec.title;
		int isize = CFG("category_icon_size").toInt();

		QString symbol, theme;

		if (customIcon == "" && it->rec.title == ctx::nameCategoryDesktop)
		{
			customIcon = CFG("icon_place_desktop");
		}

		if (!customIcon.isEmpty())
			symbol = customIcon;
		if (symbol.isEmpty() && !it->getIconPath().isEmpty())
		{
			symbol = it->getIconPath();
			QString psymbol, ptheme;
			iconLoader.parseIconParameter(symbol, psymbol, theme);
			symbol = psymbol;
			theme = ptheme;
		}

		if (!symbol.isEmpty())
		{
			QString subtype = KEY_ITEM_ICON;
			srvRequestIcon(fout, subtype, ptr, symbol, isize, theme);
		}

	}

	FOUT_CLOSE(str.ficons);

	qDebug("srv load icons done.");

}

void Loader::srvLoadItems()
{
	qDebug("srv load applications...");

	FOUT_INIT(str.fitems);

	ApplicationLoader apploader;
	apploader.insertPlaces(vitems);
	apploader.getItems(vitems);

	for (auto it : vitems)
	{
		//qDebug("test %s",QS(it->sortText(0)));

		ConfigMap cans;
		cans[KEY_TYPE] = cans[KEY_SUBTYPE] = TP_ITEM;

#define ans(name) cans[#name]=it->rec.name

		ans(iconName);
		ans(title);
		ans(genname);
		ans(loctitle);
		ans(locgenname);
		ans(command);
		ans(comment);
		ans(loccomment);
		ans(fname);

		/*
		 int count = 0;
		 for (auto c : it->rec.vcategories)
		 {
		 QString key = "vcategories" + QString::number(count++);
		 cans[key] = c;
		 }*/

		cans[KEY_ITEMTYPE] = QString::number(it->load.type);
		cans[KEY_DESKTOP] = QString::number(it->load.desktop);
		cans[KEY_SORT] = QString::number(it->sort);
		cans[KEY_DUPID] = QString::number(it->load.dupId);

		int s = ++itemSerial;
		it->load.serial = s;
		cans[KEY_SERIAL] = QString::number(s);

		srvSend(fout, cans);
	}

	FOUT_CLOSE(str.fitems);

	qDebug("srv load applications done.");

}

Loader::catrec Loader::srvCreateCategory(QString catName, CategoryLoader::category_rec& crec, ConfigMap& cans)
{
	Item * itemCat = new Item(0, Item::Order::ord_default, false);
	itemCat->rec.title = catName;
	itemCat->rec.loctitle = Config::getVTrans(catName);
	itemCat->rec.fname = crec.dirFilePath;

	if (!crec.dirFilePath.isEmpty())
	{
		ConfigMap& tmap = categoryLoader.getCategoryDirFileMap(crec.dirFile);
		itemCat->rec.loctitle = ApplicationLoader::getMapLocValue(tmap, "name");
		itemCat->rec.comment = tmap["comment"];
		itemCat->rec.loccomment = ApplicationLoader::getMapLocValue(tmap, "comment");
		itemCat->iconPath = tmap["icon"];
		CategoryLoader categoryLoader;
	}

	QString propTitle = ItemProperties(itemCat).get(ItemProperties::pn.CategoryTitle);
	if (!propTitle.isEmpty())
		itemCat->rec.loctitle = propTitle;

	cans[KEY_TYPE] = cans[KEY_SUBTYPE] = TP_ITEM;

	cans["title"] = catName;
	cans["loctitle"] = itemCat->rec.loctitle;
	cans["comment"] = itemCat->rec.comment;
	cans["loccomment"] = itemCat->rec.loccomment;
	cans["fname"] = itemCat->rec.fname;
	itemCat->load.dirFile = cans["dirfile"] = crec.dirFile;

	cans[KEY_ITEMTYPE] = QString::number(Item::ITEM_TYPE_CATEGORY);
	cans[KEY_SORT] = QString::number(Item::Order::ord_default);

	int catserial = ++itemSerial;
	cans[KEY_SERIAL] = QString::number(catserial);
	itemCat->load.serial = catserial;

	catrec cr = { itemCat, cans };
	vcatmapbyserial[catserial] = cr;
	vcatmapbyname[catName] = cr;

	//qDebug("srv add cat %i %s", catserial, QS(catName));
	vcategories.push_back(itemCat);
	vseriallist[catserial];		//create entry

	return cr;
}

void Loader::srvLoadCategories()
{
	qDebug("srv load categories...");

	FOUT_INIT(str.fcategories);
	categoryLoader.init();

	QString desktopPath = Config::getDesktopPath();

	vector<Item *> vsecond;

	int deskcount = 0;

	for (int i = 0; i <= 1; i++)
	{
		for (auto it : i == 0 ? vitems : vsecond)
		{

			if (it->load.type & Item::ITEM_TYPE_PLACE)
				continue;

			QString catName = ItemProperties(it).get(ItemProperties::pn.Category);

			if (i == 0 && it->load.desktop)
				deskcount++;

			CategoryLoader::category_rec crec;
			if (catName.isEmpty())
			{
				categoryLoader.findCategory(it, crec);
				catName = crec.category;
			}
			if (catName.isEmpty())
				continue;

			if (vcatmapbyname.find(catName) != vcatmapbyname.end())
			{
				Item * icat = vcatmapbyname[catName].icat;
				it->load.catSerial = icat->load.serial;
				continue;
			}

			if (i == 0 && crec.dirFilePath.isEmpty())
			{
				vsecond.push_back(it);
				continue;
			}

			ConfigMap cans;
			catrec cr = srvCreateCategory(catName, crec, cans);
			Item * itemCat = cr.icat;
			int cserial = itemCat->load.serial;
			it->load.catSerial = cserial;
		}
	}

	//create `Desktop` category

	if (deskcount)
	{

		ConfigMap cans;
		QString catName = ctx::nameCategoryDesktop;
		CategoryLoader::category_rec crec;
		catrec cr = srvCreateCategory(catName, crec, cans);
		Item * itemCat = cr.icat;
		int cserial = itemCat->load.serial;
		for (auto it : vitems)
		{
			if (it->load.desktop && it->load.catSerial == 0)
			{
				it->load.catSerial = cserial;
				it->categoryItem = itemCat;
			}

		}
	}

	//create `Other` category

	ConfigMap cans;
	CategoryLoader::category_rec crec;
	catrec cr = srvCreateCategory(ctx::nameCategoryOther, crec, cans);
	ctx::categoryOther = cr.icat;
	int serialOther = cr.icat->load.serial;

	for (auto it : vitems)
	{
		auto iter = vcatmapbyserial.find(it->load.catSerial);
		if (iter != vcatmapbyserial.end())
		{
			vseriallist[(*iter).second.icat->load.serial].push_back(it->load.serial);
			it->categoryItem = (*iter).second.icat;
		} else
			it->categoryItem = ctx::categoryOther;
	}

	srvRemoveDuplicates(vitems);

	for (auto e : vseriallist)
	{

		QString list;
		QString hidden;
		for (auto serial : e.second)
		{
			list = list % QString::number(serial) % ";";
		}
		auto cans = vcatmapbyserial[e.first].cans;
		cans["itemserials"] = list;
		if (e.first != serialOther)
			srvSend(fout, cans);

	}

	QString hiddenSerials;
	for (auto it : vitems)
		if (it->load.hide)
			hiddenSerials += QString::number(it->load.serial) + ";";
	if (!hiddenSerials.isEmpty())
	{
		ConfigMap cans;
		cans[KEY_TYPE] = cans[KEY_SUBTYPE] = TP_HIDDEN;
		cans["itemserials"] = hiddenSerials;
		srvSend(fout, cans);
	}

	qDebug("srv load categories done.");

	FOUT_CLOSE(str.fcategories);

}

void Loader::srvRemoveDuplicates(vector<Item*>& vitems)
{

	//auto t1 = ctx::clockmicro();
	int size = vitems.size();
	int dupId = 0;
	map<int, set<Item*>> mdup;
	for (int i1 = 0; i1 < size; i1++)
	{
		Item * it1 = vitems[i1];
		auto& r1 = it1->rec;
		QString ti1 = r1.loctitle.isEmpty() ? r1.title : r1.loctitle;
		for (int i2 = i1 + 1; i2 < size; i2++)
		{
			Item * it2 = vitems[i2];
			auto& r2 = it2->rec;
			QString ti2 = r2.loctitle.isEmpty() ? r2.title : r2.loctitle;
			if (ti1 == ti2)
			{
				bool duplicate = true;
				if (!r1.command.endsWith(r2.command) && !r2.command.endsWith(r1.command))
					duplicate = false;
				if (r1.iconName != r2.iconName)
					duplicate = false;
				//qDebug("in %.30s %.30s %i", QS(ti1), QS(ti2), duplicate);
				if (duplicate)
				{
					int dup = it1->load.dupId;
					if (dup == 0)
					{
						dup = ++dupId;
						it1->load.dupId = dup;
						mdup[dup].insert(it1);
					}
					it2->load.dupId = dup;
					mdup[dup].insert(it2);
				}
			}
		}
	}

	for (auto e : mdup)
	{
		auto &v = e.second;
		int count = v.size();

		for (int i = 0; i <= 2; i++)
		{
			//from duplicates first hide items from `Other' category, then from `Desktop`, then others
			for (auto it : v)
			{
				bool isother = it->categoryItem == ctx::categoryOther;
				bool isdesktop = it->load.desktop;
				if (i == 0 && count > 1 && isother)
				{
					it->load.hide = 1;
					count--;
				}
				if (i == 1 && count > 1 && isdesktop && !isother)
				{
					it->load.hide = 1;
					count--;
				}
				if (i == 2 && count > 1 && !isdesktop && !isother)
				{
					it->load.hide = 1;
					count--;
				}
				//qDebug("duplicate %i %-30.30s %i count %i", i, QS(it->rec.title), it->load.hide, count);
				//		QS(it->categoryItem->rec.title), it->load.hide);
				//qDebug("duplicate %i %-30.30s %i %s hidden %i", i, QS(it->rec.title), it->load.desktop, QS(it->categoryItem->rec.title),
				//		it->load.hide);
			}

		}

	}

	//auto t2 = ctx::clockmicro();
	//qDebug("remove duplicates time %lld micros", t2 - t1);

}

void Loader::srvMain()
{
	qDebug("loader started");
	ImgConv ic;
	imgconv = &ic;

	auto t1 = ctx::clockmicro();
	srvInitEnv();
	auto t2 = ctx::clockmicro();
	srvLoadItems();
	auto t3 = ctx::clockmicro();
	srvLoadCategories();
	auto t4 = ctx::clockmicro();
	srvLoadIcons();
	auto t5 = ctx::clockmicro();

	int tenv = (t2 - t1) / 1000;
	int titems = (t3 - t2) / 1000;
	int tcat = (t4 - t3) / 1000;
	int ticons = (t5 - t4) / 1000;
	int tall = (t5 - t1) / 1000;
//int trest = tall - titems - tcat - ticons - tenv;
	qDebug("loader server time:");
	qDebug("\t all   %6i ms", tall);
//qDebug("\t rest  %6i ms", trest);
	qDebug("\t env   %6i ms", tenv);
	qDebug("\t items %6i ms", titems);
	qDebug("\t categ %6i ms", tcat);
	qDebug("\t icons %6i ms", ticons);

	int iffree = 0;
	if (iffree) //exit() will free it anyway
	{
		for (auto it : vitems)
			delete it;
		for (auto it : vcategories)
			delete it;
		vitems.clear();
		vcategories.clear();
	}

	if (useCache)
		createCtlFile();

	ctx::application->quit();
}

void Loader::createCtlFile()
{
	Toolkit t;
	std::ofstream fout;
	fout.open(QS(str.fctl));
	assert(fout.is_open());
	fout << QS(t.getFileHash(str.fenv)) << "\n";
	fout << QS(t.getFileHash(str.fitems)) << "\n";
	fout << QS(t.getFileHash(str.fcategories)) << "\n";
	fout << QS(t.getFileHash(str.ficons)) << "\n";
	fout.close();

}

bool Loader::checkCtlFile()
{
	Toolkit t;
	if (!t.fexists(str.fctl))
		return false;
	vector<QString> vhash, vname;
	t.getFileLines(str.fctl, vhash);
	vname.push_back(str.fenv);
	vname.push_back(str.fitems);
	vname.push_back(str.fcategories);
	vname.push_back(str.ficons);

	int idx = 0;
	for (auto fname : vname)
	{
		QString h = t.getFileHash(fname);
		if (h != vhash[idx++])
			return false;
	}
	return true;
}

int Loader::cliAnswerApp(ConfigMap& tmap)
{
	Item * item = new Item();

#define setitem(name) item->rec.name=tmap[#name]
	setitem(iconName);
	setitem(title);
	setitem(genname);
	setitem(loctitle);
	setitem(locgenname);
	setitem(command);
	setitem(comment);
	setitem(loccomment);
	setitem(fname);

	item->load.type = tmap[KEY_ITEMTYPE].toInt();
	item->load.desktop = tmap[KEY_DESKTOP].toInt();
	item->load.dupId = tmap[KEY_DUPID].toInt();
	item->sort = tmap[KEY_SORT].toInt();
	item->separator = item->rec.title.isEmpty();
	int serial = tmap[KEY_SERIAL].toInt();
	assert(serial > 0);
	item->load.serial = serial;
	vserials[serial] = item;
	if (item->load.type & Item::ITEM_TYPE_PLACE)
		item->categoryItem = ctx::categoryPlaces;
	else
		item->categoryItem = ctx::categoryOther;

	item->addToBaseList(ctx::appList);
	item->addToGuiList();
	return 0;
}

int Loader::cliAnswerCategory(ConfigMap& tmap)
{

	Item * itemcat = new Item();

#undef setitem
#define setitem(name) itemcat->rec.name=tmap[#name]
	setitem(iconName);
	setitem(title);
	setitem(loctitle);
	setitem(comment);
	setitem(loccomment);
	setitem(fname);

	itemcat->load.type = tmap[KEY_ITEMTYPE].toInt();
	itemcat->sort = tmap[KEY_SORT].toInt();
	itemcat->separator = itemcat->rec.title == "";
	int serial = tmap[KEY_SERIAL].toInt();
	assert(serial > 0);
	itemcat->load.serial = serial;
	vserials[serial] = itemcat;
//qDebug("cli add category %i %s",serial,QS(item->rec.title));

	itemcat->addToBaseList(ctx::catList);
	itemcat->addToGuiList();

	vector<QString> v;
	tmap.split("itemserials", v);
	for (auto s : v)
	{
		Item * it = vserials[s.toInt()];
		if (!(it->load.type & Item::ITEM_TYPE_PLACE))
		{
			assert(it!=NULL);
			it->categoryItem = itemcat;
		}
	}
	return 0;
}

int Loader::cliAnswerIcon(ConfigMap& cmap)
{
	static QString search1;
	QString subtype = cmap[KEY_SUBTYPE];

//qDebug("cli icon %s", QS(pack(cmap)));

	if (subtype == "search1")
		search1 = cmap[KEY_PATH];
	if (subtype == "search2")
	{
		auto search2 = cmap[KEY_PATH];
		ctx::searchBox->setIcons(search1, search2, 16);
	}

	if (subtype.startsWith("dlg-"))
	{
		ConfigMap cdlg;
		QString path = PATH(DLG_ICONS_MAP);
		cdlg.load(path);
		cdlg[subtype] = cmap[KEY_PATH];
		cdlg.save(path);
	}

	if (subtype == "icon_toolbar_shutdown")
		ctx::buttonShutdown->setIcon(QIcon(cmap[KEY_PATH]));
	if (subtype == "icon_toolbar_lock")
		ctx::buttonLock->setIcon(QIcon(cmap[KEY_PATH]));
	if (subtype == "icon_toolbar_logout")
		ctx::buttonLogout->setIcon(QIcon(cmap[KEY_PATH]));
	if (subtype == "icon_toolbar_settings")
		ctx::buttonSettings->setIcon(QIcon(cmap[KEY_PATH]));
	if (subtype == "icon_toolbar_reload")
		ctx::buttonReload->setIcon(QIcon(cmap[KEY_PATH]));

	if (subtype == "icon_menu")
	{

		ctx::wnd->panelbt.iconPath = cmap[KEY_PATH];
		if (!ctx::wnd->panelbt.iconPath.isEmpty())
			ctx::wnd->panelbt.icon = QIcon(ctx::wnd->panelbt.iconPath);
		else
			ctx::wnd->panelbt.icon = QIcon();
	}

	QPixmap pix;
	int isize = CFG("category_icon_size").toInt();

#define caticon(xitem,xpar)	\
	\
	if(subtype==xpar) \
	{ \
		pix = iconLoader.getPixmap(cmap[KEY_PATH], isize);\
		if (!pix.isNull())\
			xitem->gui.icon->setPixmap(pix);\
	}

	caticon(ctx::categoryAll, "icon_category_all");
	caticon(ctx::categoryFavorites, "icon_category_favorites");
	caticon(ctx::categoryRecent, "icon_category_recent");
	caticon(ctx::categoryPlaces, "icon_category_places");
	caticon(ctx::categoryOther, "icon_category_other");

	if (subtype == KEY_ITEM_ICON)
	{
		int serial = cmap[KEY_SERIAL].toInt();
		Item * it = vserials[serial];
		if (it == NULL)
			qDebug("item not found by serial %i", serial);
		assert(it != NULL);
		QString path = cmap[KEY_PATH];
		int isize = cmap[KEY_SIZE].toInt();
		if (!path.isEmpty())
		{
			assert(path.contains("/"));
			assert(isize > 0);
			it->rec.iconName = cmap[KEY_SYMBOL];
			it->iconPath = path;

			QPixmap pix;
			QIcon icon = QIcon(path);
			pix = icon.pixmap(isize, isize);
			it->gui.icon->setPixmap(pix);

		}
	}

	return 0;
}

int Loader::cliAnswerHidden(ConfigMap& cmap)
{
	QString serials = ";" + cmap["itemserials"];
	for (auto it : ctx::appList->vitems())
		if (serials.contains(";" + QString::number(it->load.serial) + ";"))
		{
			it->load.hide = 1;
			//qDebug("hide %s",QS(it->rec.title));
		}
	return 0;
}

int Loader::cliLoadItems(bool nowait)
{
	ctx::appList->hide();
	int res = cliProcessAnswerFile(str.fitems, nowait);
	ctx::appList->show();
	if (res)
		progress.items = true;

	return res;
}

int Loader::cliLoadCategories(bool nowait)
{
	ctx::catList->hide();
	int res = cliProcessAnswerFile(str.fcategories, nowait);
	if (!res)
	{
		ctx::catList->show();
		return 0;
	}

	int countother = 0;
	for (auto it : ctx::appList->vitems())
	{
		if (it->categoryItem == NULL)
			it->categoryItem = ctx::categoryOther;

		if (it->categoryItem == ctx::categoryOther)
		{
			//qDebug("other[1] %s",QS(it->rec.title));
			if (!it->load.hide)
			{
				countother++;
				//qDebug("other[2] %s",QS(it->rec.title));
			}
		}

	}

	ctx::catList->reorder(Item::Order::BY_TEXT);

	if (countother == 0)
	{
		int row = ctx::categoryOther->row();
		ctx::categoryOther->gui.hidden = true;
		ctx::catList->uiList()->setRowHidden(row, true);
		//qDebug("hide other");
	}

	ctx::catList->show();

	ctx::appList->hide();
	ctx::appList->reorder(Item::Order::BY_TEXT);
	ctx::appList->show();

	progress.categories = true;

	return 1;
}

int Loader::cliLoadIcons(bool nowait)
{
	int res = cliProcessAnswerFile(str.ficons, nowait);
	if (!res)
		return res;
	cliPostActions();
	progress.icons = true;
	return res;
}

int Loader::cliLoadEnv(bool nowait)
{
	int res = cliProcessAnswerFile(str.fenv, nowait);
	if (!res)
		return res;
	progress.env = true;
	return res;
}

void Loader::cliPostActions()
{
	for (auto it : ctx::appList->vitems())
	{
		it->setTooltip();
	}
	for (auto it : ctx::catList->vitems())
	{
		it->setTooltip();
	}

}

int Loader::cliProcessAnswerFile(QString fname, bool nowait)
{
	auto t1 = ctx::clockmicro();
	Toolkit toolkit;
	if (vwait.find(fname) == vwait.end())
		vwait[fname] = t1;
	while (!toolkit.fexists(fname))
	{
		if (nowait)
			return 0;
		//qDebug("client wait %s", QS(fname));
		usleep(1000);
	}

	auto t2 = ctx::clockmicro();

	std::ifstream fin;
	fin.open(QS(fname));
	if (!fin.is_open())
	{
		qDebug("open file error %s", QS(fname));
		exit(1);
	}

	string sline;
	while (std::getline(fin, sline))
	{
		QString line = decode(sline.c_str());
		//qDebug("\ndecode %s", QS(line));

		ConfigMap cmap;
		unpack(line, cmap);
		QString type = cmap[KEY_TYPE];

		if (type == TP_HIDDEN)
			cliAnswerHidden(cmap);
		if (type == TP_ICON)
			cliAnswerIcon(cmap);
		if (type == TP_ITEM)
		{
			int itemtype = cmap[KEY_ITEMTYPE].toInt();
			if (itemtype & Item::ITEM_TYPE_CATEGORY)
				cliAnswerCategory(cmap);
			else
				cliAnswerApp(cmap);
		}

	}
	fin.close();

	auto t3 = ctx::clockmicro();

	int tall = (t3 - t1) / 1000;
	int twait = (t2 - t1) / 1000;
	if (nowait)
		twait = (t2 - vwait[fname]) / 1000;
	qDebug("loader client %-22.22s time ms: wait %4i all %4i", QS(QFileInfo(fname).fileName()), twait, tall);
	return 1;
}

void Loader::closeServer()
{
	if (!srvpid)
		return;
	for (;;)
	{
		int k = kill(srvpid, 0);
		process->waitForFinished();
		if (k)
			break;
	}
	srvpid = 0;
}

void Loader::onLoaderEvent(UserEvent * uev)
{
	UserEvent * ue;
	if (uev == NULL)
		ue = new UserEvent(UserEventType::LOADER);
	else
		ue = uev;
	auto& rec = this->progress;

	if (!rec.env)
		ctx::loader->cliLoadEnv(true);
	if (rec.env && !rec.items)
		ctx::loader->cliLoadItems(true);
	if (rec.items && !rec.categories)
		ctx::loader->cliLoadCategories(true);
	if (rec.categories && !rec.icons)
		ctx::loader->cliLoadIcons(true);

	if (!rec.items || !rec.categories || !rec.icons)
	{
		//qDebug("resend %i %i %i",rec.items, rec.categories,rec.icons);
		auto ue1 = new UserEvent(UserEventType::LOADER);
		//ue1->custom.load = ue->custom.load;
		ue1->postLater(5);
	} else
	{
		assert(ctx::loader!=NULL);
		ctx::loader->closeServer();
		delete ctx::loader;
		ctx::loader = NULL;
		(new UserEvent(UserEventType::SET_PANEL_BUTTON))->postLater(0);

		ctx::wnd->setStartupRow(ctx::wnd->showForce ? 0 : -1);
		ctx::keyListener.setHotKeyFromString(CFG("hotkey"));
		qDebug("onLoaderEvent finish time %lld", (ctx::clockmicro() - ctx::thread_data.tstart) / 1000);
		ctx::isReloading = 0;
	}
	if (uev == NULL)
		delete ue;

}

void Loader::clearLoaderFiles()
{
#define DELCHK(f)	QFile(f).remove();assert(!QFile(f).exists())
	QString ext[] = { "", ".tmp" };
	for (auto e : ext)
	{
		DELCHK(str.fenv + e);
		DELCHK(str.fitems + e);
		DELCHK(str.fcategories + e);
		DELCHK(str.ficons + e);
	}
	DELCHK(str.fctl);
}

void Loader::loadMapWithFilter(QString fname, ConfigMap& cmap)
{
	static QString ln = ctx::localeName;
	static QString loc1 = "[" % ln.mid(0, 2) % "]";
	static QString loc2 = "[" % ln.mid(0, 5) % "]";

	FILE *f = fopen(QS(fname), "r");
	if (f == NULL)
	{
		perror(QS(fname));
		return;
	}

	QTextStream in(f, QIODevice::ReadOnly);
	in.setCodec("UTF-8");
	QString line;
	while (!in.atEnd())
	{
		line = in.readLine();

		if (!line.startsWith("X-") && !line.startsWith("MimeType"))
		{
			int pos = line.indexOf("=");
			if (pos >= 0)
			{
				QString key = line.mid(0, pos).trimmed();
				int p = key.indexOf("[");
				if (p < 0 || key.indexOf(loc1) >= 0 || key.indexOf(loc2) >= 0)
				{
					cmap[key] = line.mid(pos + 1).trimmed();
					//qDebug("line %s",QS(line));
				}
			}
		}

	}
	fclose(f);
}
