#include "config.h"
#include "configmap.h"
#include "mainwindow.h"
#include "parameterform.h"
#include "toolkit.h"
#include "itemlist.h"
#include <QListWidgetItem>
#include <QLabel>
#include <QMessageBox>
#include <QCloseEvent>
#include <QProcess>
#include <assert.h>
#include "ctx.h"
#include "x11util.h"
#include "userevent.h"
#include "searchbox.h"
#include <unistd.h>
#include <sstream>
#include <fstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <errno.h>
#include "resource.h"

using namespace std;
using namespace ctx;

std::map<QString, QString> Config::vtrans;

Config::Config()
{
}

Config::~Config()
{
	for (auto it : vitems)
		delete it;
}

int Config::newItem(int parentid, QString name, int type, QString value, QString label, QString lov, bool mandatory, bool reload)
{
	static int idcounter = 2; //>1
	assert(parentid != 0 || vitems.size() == 0);
	ConfigItem * it = new ConfigItem();
	Toolkit t;

	it->id = idcounter++;
	it->parentid = parentid;
	it->name = name;
	it->labelorg = label;
	it->type = type;
	it->value = value;
	it->mandatory = mandatory;
	it->reloadreq = reload;
	if (lov == "hotkey")
	{
		it->hotkey = true;
	} else if (lov.contains(";"))
	{
		t.tokenize(lov, it->validitems, ";");
		it->mandatory = true;
	} else if (lov.contains(":"))
	{
		t.tokenize(lov, it->validitems, ":");
		it->rangeMin = it->validitems[0];
		it->rangeMax = it->validitems[1];
		it->validitems.clear();
		it->mandatory = true;
	}

	if (it->labelorg == "")
	{
		it->labelorg = it->name;
		it->isCategory = true;
	}

	QString ltxt = it->labelorg;
	auto spl = ltxt.split("|");
	if (spl.length() == 2)
		ltxt = spl[1].trimmed();
	it->ord += ltxt.count("#");
	ltxt = ltxt.replace("#", "");
	it->ord -= ltxt.count("@");
	ltxt = ltxt.replace("@", "");

	it->label = ltxt;

	vitems.push_back(it);
	//idmap[it->id] = it;
	//qDebug("new item %i %i %s",it->id,it->parentid,QS(it->label));

	return it->id;
}

void Config::setDefaultConfig(ConfigMap& cmap)
{
	//idmap.clear();
	//vitems.clear();
	assert(vitems.size() == 0);
	int root = newItem(0, tr("Parameters"));

	int CHR = 0;
	int INT = ParameterForm::Item::TYPE_INT;
	int c, cp, cl;

	QString iconSizes = "48;32;24;22;16";
	QString booleanList = QString(ParameterForm::YES) + ";" + QString(ParameterForm::NO);
	QString viewMode = "list;icons;compact";
	QString rowHeight = "1:500";
	QString itemWidth = "1:2000";

	cl = newItem(root, tr("List of items"));

	c = newItem(cl, tr("Applications"));

	newItem(c, "app_view_type", CHR, "list", tr("View type"), viewMode);
	newItem(c, "app_item_height", INT, "45", tr("Row height"), rowHeight);
	newItem(c, "app_item_width", INT, "64", tr("Item width for icon view mode"), itemWidth);
	newItem(c, "app_icon_size", INT, "32", tr("Icon size"), iconSizes, true, true);
	newItem(c, "app_item_with_comment", CHR, ParameterForm::YES, tr("Show comments"), booleanList);
	newItem(c, "app_item_show_text", CHR, ParameterForm::YES, tr("Show texts"), booleanList);
	newItem(c, "app_item_show_icon", CHR, ParameterForm::YES, tr("Show icons"), booleanList);
	newItem(c, "app_enable_tooltips", CHR, ParameterForm::YES, tr("Show tooltips"), booleanList);
	newItem(c, "use_generic_names", CHR, ParameterForm::NO, tr("Use generic names"), booleanList);

	c = newItem(cl, tr("Categories"));

	newItem(c, "category_view_type", CHR, "list", tr("View type"), viewMode);
	newItem(c, "category_item_height", INT, "30", tr("Row height"), rowHeight);
	newItem(c, "category_icon_size", INT, "24", tr("Icon size"), iconSizes, true, true);
	newItem(c, "category_item_width", INT, "64", tr("Item width for icon view mode"), itemWidth);
	newItem(c, "category_item_show_text", CHR, ParameterForm::YES, tr("Show texts"), booleanList);
	newItem(c, "category_item_show_icon", CHR, ParameterForm::YES, tr("Show icons"), booleanList);
	newItem(c, "category_enable_tooltips", CHR, ParameterForm::YES, tr("Show tooltips"), booleanList);
	newItem(c, "categories_on_the_right", CHR, ParameterForm::NO, tr("Display on the right"), booleanList);

	c = newItem(cl, tr("Places"));

	newItem(c, "place_item_height", INT, "28", tr("Row height"), rowHeight, true, true);
	newItem(c, "place_icon_size", INT, "24", tr("Icon size"), iconSizes, true, true);

	c = newItem(root, tr("Commands"));

	newItem(c, "command_file_manager", CHR, "caja", tr("File manager"));
	newItem(c, "command_shutdown", CHR, "mate-session-save --shutdown-dialog", tr("Shutown"));
	newItem(c, "command_lock_screen", CHR, "mate-screensaver-command -l", tr("Lock screen"));
	newItem(c, "command_logout", CHR, "mate-session-save --logout-dialog", tr("Session logout"));

	cp = newItem(c, tr("places"));

	newItem(cp, "command_place_computer", CHR, "$filemanager$ computer:", tr("Computer"), "", true, true);
	newItem(cp, "command_place_home", CHR, "$filemanager$ ~", tr("Home"), "", true, true);
	newItem(cp, "command_place_network", CHR, "$filemanager$ network:", tr("Network"), "", true, true);
	newItem(cp, "command_place_trash", CHR, "$filemanager$ trash:", tr("Trash"), "", true, true);
	newItem(cp, "command_place_desktop", CHR, "$filemanager$ $desktop$", tr("Desktop"), "", true, true);

	c = newItem(root, tr("Paths"));

	newItem(c, "desktop_additional_paths", CHR, ";", tr("Additional paths for .desktop files"), "", false, true);
	newItem(c, "category_additional_files", CHR, ";", tr("Additional paths for .directory files"), "", false, true);
	newItem(c, "icons_additional_paths", CHR, ";", tr("Additional paths for icons"), "", false, true);

	c = newItem(root, tr("Icons"));

	newItem(c, "icons_preferred_themes", CHR, ";", tr("Preferred themes"), "", false, true);
	newItem(c, "icon_default", CHR, "computer", tr("Default icon"), "", true, true);
	newItem(c, "icon_menu", CHR, "system-run", tr("Menu icon on the panel"), "", false, true);

	cp = newItem(c, tr("buttons"));

	newItem(cp, "icon_toolbar_settings", CHR, "gnome-settings", tr("`Preferences` button"), "", true, true);
	newItem(cp, "icon_toolbar_reload", CHR, "gtk-refresh", tr("`Reload menu` button"), "", true, true);
	newItem(cp, "icon_toolbar_shutdown", CHR, "system-shutdown;oxygen", tr("`Shutdown` button"), "", true, true);
	newItem(cp, "icon_toolbar_lock", CHR, "system-lock-screen", tr("`Lock screen` button"), "", true, true);
	newItem(cp, "icon_toolbar_logout", CHR, "system-log-out", tr("`Logout` button"), "", true, true);

	cp = newItem(c, tr("categories"));

	newItem(cp, "icon_category_all", CHR, "gtk-select-all", tr("Category `All`"), "", true, true);
	newItem(cp, "icon_category_favorites", CHR, "emblem-favorite", tr("Category `Favorites`"), "", true, true);
	newItem(cp, "icon_category_other", CHR, "computer", tr("Category `Other`"), "", true, true);
	newItem(cp, "icon_category_recent", CHR, "document-open-recent;matefaenza", tr("Category `Recently used`"), "", true, true);
	newItem(cp, "icon_category_places", CHR, "folder", tr("Category `Places`"), "", true, true);

	cp = newItem(c, tr("places"));

	newItem(cp, "icon_place_computer", CHR, "computer", tr("Computer"), "", true, true);
	newItem(cp, "icon_place_home", CHR, "folder-home", tr("Home"), "", true, true);
	newItem(cp, "icon_place_network", CHR, "gtk-network", tr("Network"), "", true, true);
	newItem(cp, "icon_place_trash", CHR, "emptytrash", tr("Trash"), "", true, true);
	newItem(cp, "icon_place_desktop", CHR, "desktop", tr("Desktop"), "", true, true);

	c = newItem(root, "@" + tr("Menu"));

	newItem(c, "menu_sync_delay_ms", INT, "200", tr("Menu synchronization delay [ms]"), "0:3000");
	newItem(c, "include", CHR, ";", tr("Include config files"));
	newItem(c, "css_file", CHR, PATH(CSS_DEFAULT), tr("Style sheet file"));
	newItem(c, "menu_label", CHR, "Menu", tr("Menu label on the panel"), "", false, true);
	newItem(c, "hotkey", CHR, "", tr("Hot key"), "hotkey");

	for (auto it : vitems)
	{
		if (!it->isCategory)
			cmap[it->name] = it->value;
	}
}

void Config::inheritIncludes(ConfigMap& cfgorg, ConfigMap& cfg)
{
	std::vector<QString> includes;
	cfgorg.copyTo(cfg);
	cfg.split("include", includes);
	for (QString incfile : includes)
	{
		QString incpath = PATH(BASE)+ incfile;
		incpath.replace("//", "/");
		if (QFile(incpath).exists())
		cfg.load(incpath);
	}
}

vector<ConfigItem *> * Config::items()
{
	return &vitems;
}

QString Config::getDesktopPath()
{
	Toolkit toolkit;
	QString desktopPath = "~/Desktop";
	vector<QString> v;
	QString home = QDir::homePath();
	toolkit.getFileLines(home + "/.config/user-dirs.dirs", v);
	for (auto s : v)
	{
		if (s.startsWith("XDG_DESKTOP_DIR="))
		{
			s.replace("XDG_DESKTOP_DIR=", "");
			s.replace("\"", "");
			desktopPath = s;
			break;
		}
	}
	desktopPath.replace("$HOME", home);
	desktopPath.replace("~/", home + "/");
	return desktopPath;
}

void Config::initTranslation()
{
	static QTranslator translator;

	for (int langlen = 2; langlen <= 5; langlen += 3)
	{
		QString transFile = PATH(TRANSLATIONS)+ "/" + "trans-" + ctx::localeName.mid(0, langlen) + ".qm";
		if (QFile(transFile).exists())
		{
			//qDebug("translation from %s", QS(transFile));
			translator.load(transFile);
			ctx::application->installTranslator(&translator);
			break;
		}
	}

	ctx::nameCategoryAll = "All";
	ctx::nameCategoryFavorites = "Favorites";
	ctx::nameCategoryRecent = "Recently used";
	ctx::nameCategoryOther = "Other";
	ctx::nameCategoryPlaces = "Places";
	ctx::nameCategoryDesktop = "Desktop";

	vtrans[ctx::nameCategoryAll] = tr("All");
	vtrans[ctx::nameCategoryFavorites] = tr("Favorites");
	vtrans[ctx::nameCategoryRecent] = tr("Recently used");
	vtrans[ctx::nameCategoryOther] = tr("Other");
	vtrans[ctx::nameCategoryPlaces] = tr("Places");
	vtrans[ctx::nameCategoryDesktop] = tr("Desktop");

	ctx::menuProperties = tr("Properties");
	ctx::menuAddToDesktop = tr("Add to desktop");
	ctx::menuAddToFavorites = tr("Add to favorites");
	ctx::menuRemoveFromFavorites = tr("Remove from favorites");
	ctx::menuSetDefaultCategory = tr("Set as default");

	ctx::places =
	{
		{	tr("Computer"), "place_computer"} //
		,
		{	tr("Home folder"), "place_home"} //
		,
		{	tr("Network"), "place_network"} //
		,
		{	tr("Desktop"), "place_desktop"} //
		,
		{	tr("Trash"), "place_trash"} //
	};

	ParameterForm::YES = "yes";
	ParameterForm::NO = "no";
	ParameterForm::NONE = "none";

	vtrans["yes"] = tr("yes");
	vtrans["no"] = tr("no");
	vtrans["none"] = tr("none");
	vtrans["list"] = tr("list");
	vtrans["icons"] = tr("icons");
	vtrans["compact"] = tr("compact");
	vtrans["Icon:"] = tr("Icon:");
	vtrans["Category:"] = tr("Category:");

	vtrans["Error"] = tr("Error");
	vtrans["validTextAndIcon"] = tr("Cannot disable disable both values simultaneously: `Show text` and `Show icon`");
}

QString Config::getVTrans(QString txt, int reverse)
{
	if (reverse)
	{
		for (auto v : vtrans)
			if (txt == v.second)
				return v.first;
		return txt;
	}
	QString trtxt = vtrans[txt];
	if (trtxt == "")
		trtxt = txt;
//	qDebug("trans %s %s", QS(txt), QS(trtxt));
	return trtxt;
}

#include "a1menu.res.gz.h"
#define resblob  _tmp_a1menu_res_gz
#define reslen   _tmp_a1menu_res_gz_len

void Config::installDefaultFiles()
{
	std::vector<Resource::resfile> vfiles;
	Resource().openFromBuffer((char*) resblob, reslen, vfiles);

	Toolkit toolkit;
	QString cssname = QFileInfo(PATH(CSS_DEFAULT)).fileName();

	for (auto rf : vfiles)
	{

		QString dst = "";
		if (rf.fname.endsWith(cssname))
		{
			dst = PATH(CSS_DEFAULT);
		}
		if (rf.fname.contains("ts/") && (rf.fname.endsWith(".qm") || rf.fname.endsWith(".ts")))
		{

			dst = PATH(TRANSLATIONS)+"/"+QFileInfo(rf.fname).fileName();
		}

		if (dst != "")
		{
			if (toolkit.getHash(rf.content.c_str(), rf.content.length()) != toolkit.getFileHash(dst))
			{
				qDebug(" instal %s", QS(dst));

				std::ofstream fout;
				fout.open(QS(dst), std::ios::out | std::ios::binary);
				fout.write(rf.content.c_str(), rf.content.length());
				fout.close();
			}
		}

	}

}

QString Config::getStyleFile()
{
	//QString val((const char *)cssblob);

	QString val;
	QString fname = CFG("css_file");

	if (fname == "")
		return "";
	if (!fname.contains("/"))
		fname = PATH(BASE)+ "/" + fname;

	val = Toolkit().getFileText(fname).trimmed();

	//remove comments
	for (;;)
	{
		int p1 = val.indexOf("/*");
		int p2 = -1;
		if (p1 >= 0)
		{
			p2 = val.indexOf("*/", p1);
		}
		if (p2 < 0)
			break;
		val = val.mid(0, p1) + val.mid(p2 + 2);
	}

	//apply #define
	for (;;)
	{
		int found = 0;
		QString define = "#define";
		auto spl = val.split("\n", QString::SkipEmptyParts);
		for (auto line : spl)
		{
			if (val.contains(define))
			{
				auto sp = line.split(" ", QString::SkipEmptyParts);
				//qDebug("define %s",QS(sp[2]));
				if (sp.length() > 2 && sp[0] == define)
				{
					QString symbol = sp[1];
					int p = line.indexOf(" " + symbol + " ");
					QString symval = line.mid(p + symbol.length() + 2).trimmed();
					if (symval.contains(define))
						break;
					val.replace(line, "");
					val.replace(symbol, symval);
					found = 1;
					break;
				}
			}
		}
		if (found == 0)
			break;
	}
	return val;
}

namespace consts
{
char MSG_REFRESH = 'r';
char MSG_RELOAD = 'x';
char MSG_DONE = 'e';
}

int Config::runDialogProcess()
{
	static QMutex mutex;
	Toolkit toolkit;
	Toolkit::Locker ml(&mutex);
	if (!ml.isLocked())
	{
		qDebug("config dialog already opened");
		return -1;
	}

	int sockets[2];
	if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0, sockets) < 0)
	{
		perror("opening stream socket pair");
		return -2;
	}

	ctx::wnd->setShowForce(true);
	ctx::keyListener.stop();
	ctx::keyListener.lockListener(1);
	Config config;
	QString cfgFile = PATH(CFG_FILE);
	ctx::cfgorg.save(cfgFile);
	QString h1 = toolkit.getFileHash(cfgFile);

	QProcess process;
	QStringList args;
	args.append(ctx::procParConfig);
	args.append(QString::number(sockets[1]));
	process.setProcessChannelMode(QProcess::ForwardedChannels);
	process.start(ctx::thread_data.argv[0], args);
	int pid = process.pid();
	int wres = 0;
	int prevReloading = ctx::isReloading;
	int iofd = sockets[0];

	for (int i = 0;; i++)
	{
		//wres not reliable?
		wres += process.waitForFinished(30);
		int k = kill(pid, 0);
		if (k | wres)
		{
			wres += process.waitForFinished(0);
			//qDebug("exit pform status kill %i wait %i", k, wres);
			break;
		}

		if (prevReloading != ctx::isReloading)
		{
			prevReloading = ctx::isReloading;
			if (ctx::isReloading == 0)
			{
				char c = consts::MSG_DONE;
				write(iofd, &c, 1);

			}
		}
		QApplication::processEvents();

		if (ctx::isReloading)
			continue;

		char answer;
		int n = read(iofd, &answer, 1);
		//qDebug("read %i %c", n, answer);
		if (n < 1)
			continue;

		qDebug("received signal <%c>", answer);
		if (answer == consts::MSG_REFRESH || answer == consts::MSG_RELOAD)
		{
			/*
			if (answer == consts::MSG_RELOAD)
			{
				toolkit.removeRecursively(PATH(CACHE));
			}
			*/
			h1 = toolkit.getFileHash(cfgFile);
			ctx::isReloading = 1;
			auto ue=new UserEvent(UserEventType::IPC_RELOAD);
			ue->custom.clearCache=answer == consts::MSG_RELOAD;
			ue->postLater(0);
		}

	}
	close(sockets[0]);
	close(sockets[1]);

	QString h2 = toolkit.getFileHash(cfgFile);
	if (h1 != h2)
	{
		(new UserEvent(UserEventType::IPC_RELOAD))->postLater(0);
	}

	ctx::wnd->setStartupRow();
	ctx::wnd->setShowForce(false);
	ctx::keyListener.lockListener(0);
	ctx::keyListener.setHotKeyFromString(CFG("hotkey"));
	return 0;
}

class PFCallback: public ParameterFormCallback
{
public:
	int iofd;

	bool parameterFormCallback(ParameterForm * pf, int buttonCode)
	{
		int bok = buttonCode == QDialogButtonBox::Save;
		int bapply = buttonCode == QDialogButtonBox::Apply;
		int breload = buttonCode == QDialogButtonBox::Reset;

		if (!(bok || bapply || breload))
			return false;
		//qDebug("parameterFormCallback apply %i", buttonCode);
		if (bok || bapply)
		{

			int res = pf->validateForm();
			if (res < 0)
				return false;
			ConfigMap cmap;
			for (auto e : pf->getValues())
				cmap[e.first] = e.second;

			//extra validations
			QString arr[] = { "app", "category" };
			for (auto pref : arr)
				if (cmap[pref + "_item_show_text"] != ParameterForm::YES && cmap[pref + "_item_show_icon"] != ParameterForm::YES)
				{
					QMessageBox::critical(0, Config::getVTrans("Error"), Config::getVTrans("validTextAndIcon"));
					return false;
				}
			//
			cmap.save(PATH(CFG_FILE));
		}

		if (iofd > 0 && (bapply || breload))
		{
			pf->setCursor(Qt::WaitCursor);

			char c;
			for (;;)
				if (read(iofd, &c, 1) < 1)
					break;

			c = breload ? consts::MSG_RELOAD : consts::MSG_REFRESH;
			write(iofd, &c, 1);

			for (int i = 0; i < 30; i++)
			{
				if (read(iofd, &c, 1) == 1)
					break;
				usleep(100 * 1000);
			}
			pf->setCursor(Qt::ArrowCursor);
		}

		if (bok)
			pf->accept();

		return true;
	}
};

int Config::runDialogProcessProc(int iofd)
{

	//qDebug("parameter form process pid %i", getpid());
	ctx::application = new QApplication(ctx::thread_data.argc, ctx::thread_data.argv);
	Config::initTranslation();
	ParameterForm pform;
	Config config;
	ConfigMap cmapdef, cmapfile;

	PFCallback callback;
	pform.setCallback(&callback);
	callback.iofd = iofd;

	//QIcon::setThemeName("matefaenza");

	ConfigMap cdlg;
	QString path = PATH(DLG_ICONS_MAP);
	cdlg.load(path);

	pform.getButtonBox()->button(QDialogButtonBox::Reset)->setIcon(QIcon(cdlg["dlg-reset"]));
	pform.getButtonBox()->button(QDialogButtonBox::Cancel)->setIcon(QIcon(cdlg["dlg-cancel"]));
	pform.getButtonBox()->button(QDialogButtonBox::Save)->setIcon(QIcon(cdlg["dlg-save"]));
	pform.getButtonBox()->button(QDialogButtonBox::Apply)->setIcon(QIcon(cdlg["dlg-apply"]));

	//pform.getButtonBox()->button(QDialogButtonBox::Reset)->setIcon(QIcon::fromTheme("gtk-refresh"));
	//pform.getButtonBox()->button(QDialogButtonBox::Cancel)->setIcon(QIcon::fromTheme("gtk-close"));
	//pform.getButtonBox()->button(QDialogButtonBox::Save)->setIcon(QIcon::fromTheme("gtk-save"));
	//pform.getButtonBox()->button(QDialogButtonBox::Apply)->setIcon(QIcon::fromTheme("gtk-apply"));

	ctx::palorg = ctx::application->palette();
	config.setDefaultConfig(cmapdef);
	if (QFile(PATH(CFG_FILE)).exists())
		cmapfile.load(PATH(CFG_FILE));
	cmapfile.copyTo(cmapdef);
	for (auto it : *config.items())
	{
		it->value = cmapdef[it->name];
		pform.addItem(it);
	}
	pform.runDialog();
	close(callback.iofd);
	return 0;
}

