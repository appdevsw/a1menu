#include "categoryloader.h"
#include "iconloader.h"
#include "loader.h"
#include "item.h"
#include "ctx.h"
#include "configmap.h"
#include "toolkit.h"
#include <assert.h>
#include <QDir>
#include <QFileInfo>

using namespace std;
using namespace ctx;

#define catEnableDebug 0
#define catDebug if(catEnableDebug)qDebug

CategoryLoader::CategoryLoader()
{
}

CategoryLoader::~CategoryLoader()
{
	free();
}

void CategoryLoader::xmlFindMenuEntries(menu_file& mf, QDomElement& x, int level)
{
	QDomNodeList xlist = x.childNodes();
	menu_file_pos mfp;
	catDebug("%*s %s", level * 3, "", QS(x.tagName()));
	for (int i = 0; i < (int) xlist.length(); i++)
	{
		QDomElement x = xlist.item(i).toElement();
		QString tag = x.tagName();
		if (tag.isEmpty())
			continue;
		if (tag == "Menu")
			xmlFindMenuEntries(mf, x, level + 1);
		else
		catDebug("%*s %s = %s", level * 3 + 1, "", QS(x.tagName()), QS(x.text()));

		if (tag == "Name")
		{
			//assert(mfp.xname.isNull());
			mfp.xname = x;
		}
		if (tag == "Include")
			mfp.xinclude = x;
		if (tag == "Directory")
			mfp.dirFile = x.text();
	}
	if (!mfp.xname.isNull())
	{
		//qDebug("found %s %s",QS(mfp.xname.text()),QS(mfp.dirFile));
		menu_file_pos* mfpptr = new menu_file_pos();
		*mfpptr = mfp;
		mf.xlist.push_back(mfpptr);
	}
}

void CategoryLoader::init()
{
	vector<QString> vfiles;
	vector<QString> adddirs;
	CFGPTR()->split("category_additional_files", adddirs);
	for (QString apath : adddirs)
	{
		vfiles.push_back(apath);
	}
	QString base = "/etc/xdg/menus/";

	vfiles.push_back(base + "mate-applications.menu");
	vfiles.push_back(base + "mate-settings.menu");
	vfiles.push_back(base + "mate-screensavers.menu");
	//vfiles.push_back(base + "matecc.menu");

	//test only
	if (!Toolkit().fexists(base + "mate-applications.menu"))
	{
		vfiles.push_back(base + "xfce-applications.menu");
		vfiles.push_back(base + "xfce-settings-manager.menu");
	}

	vfiles.push_back(QDir::homePath() + "/.config/menus/mate-applications.menu");

	vector<QString> addfiles;

	for (QString fname : vfiles)
	{
		QDomDocument domDocument;
		QDomElement root;
		QString errorStr;
		int errorLine;
		int errorColumn;

		menu_file * mf = new menu_file();
		mf->fname = fname;
		QFile f(fname);

		if (!domDocument.setContent(&f, true, &errorStr, &errorLine, &errorColumn))
		{
			catDebug("findCategoryName error! %s", QS(fname));
			delete mf;
			continue;
		}
		root = domDocument.documentElement();
		catDebug("file %s", QS(fname));
		xmlFindMenuEntries(*mf, root, 0);
		f.close();
		categoryFiles.push_back(mf);
	}
}

QString CategoryLoader::vectorToString(vector<QString> &v)
{
	QString buf;
	for (auto s : v)
	{
		buf.append(s);
		buf.append(";");
	}
	return buf;
}

void printset_(QString level, const char* desc, const cset& v)
{
	if (!catEnableDebug)
		return;
	QString str = desc;
	str += ":";
	for (auto e : v)
	{
		str += "<";
		str += e.toStdString().c_str();
		str += ">";
	}
	catDebug("%s%s", QS(level), QS(str));
}

//#define printset(level,desc,v) printset_(level,desc,v)
#define printset(level,desc,v)

QString set2buf(const cset& v)
{
	QString str = "[";
	int count = 0;
	for (auto e : v)
	{
		if (count++ > 0)
			str += ";";
		str += e.toStdString().c_str();
	}
	str += "]";
	return str;
}

void CategoryLoader::findCategory(Item * item, category_rec& rec)
{
	//if (item->rec.title != "Mouse")
	//	return;
	//if (!item->rec.title.startsWith("Eclipse"))
	//	return;
	//if (!item->rec.fname.contains("libreoffice-math.desktop"))
	//	return;

	catDebug("\n---------------------\nFind category for %s", QS(item->rec.title));
	vector<QString>& categories = item->rec.vcategories;
	loadCategoryDirFiles();

	cset catset;
	catset.insert(categories.begin(), categories.end());
	vector<menu_file_pos *> found;
	for (menu_file * mf : categoryFiles)
	{
		for (menu_file_pos * mfp : mf->xlist)
		{
			catDebug("\n%s %s", QS(mf->fname), QS(mfp->xname.text()));
			cset cat = xmlProcessInclude(item, mfp, mfp->xinclude, catset, 0);
			QString buf;
			if (!cat.empty())
			{
				catDebug("found: %s %s %s", QS(mf->fname), QS(set2buf(cat)), QS(mfp->dirFile));
				mfp->resultset = cat;
				found.push_back(mfp);
			}

		}
	}
	vector<menu_file_pos *> orgfound;
	set<int> doneset;

	//first choose categories from the original set
	for (auto orgcat : item->rec.vcategories)
	{
		int idx = 0;
		for (auto mfp : found)
		{
			if (mfp->resultset.count(orgcat) > 0)
			{
				orgfound.push_back(mfp);
				doneset.insert(idx);
			}
			idx++;
		}
	}

	//then others
	int idx = 0;
	for (auto mfp : found)
	{
		if (doneset.find(idx) == doneset.end())
			orgfound.push_back(mfp);
		idx++;
	}

	//QString lastName, lastDirFile;
	//catDebug("Item %s %s", QS(item->rec.title), QS(catbuf));
	category_rec any, anyf;
	for (auto mfp : orgfound)
	{
		QString cat = mfp->xname.text();
		catDebug("found:: %s", QS(cat));
		if (cat == "Other" || cat == "More")
			continue;
		any=
		{	cat,mfp->dirFile};
		catDebug(" category %s", QS(cat));
		rec.category = cat;
		rec.dirFile = mfp->dirFile;
		if (rec.dirFile != "")
		{
			anyf = any;
			rec.dirFilePath = getDirFileByShortName(rec.dirFile);
			if (!rec.dirFilePath.isEmpty())
			{
				return;
			}
		}
	}

	if (anyf.category != "")
	{
		rec = anyf;
		return;
	}
	if (any.category != "")
	{
		rec = any;
		return;
	}

	rec.category = "";

}

void CategoryLoader::setIntersect(cset& s1, const cset& s2)
{
	if (s1.size() == 0 || s2.size() == 0)
		return;

	for (cset::iterator it = s1.begin(); it != s1.end();)
	{
		if (s2.find(*it) == s2.end())
			s1.erase(it++);
		else
			++it;
	}
}

void CategoryLoader::setSubtract(cset& s1, const cset& s2)
{
	if (s1.size() == 0 && s2.size() == 0)
		return;
	for (cset::iterator it = s1.begin(); it != s1.end();)
	{
		if (s2.find(*it) != s2.end())
			s1.erase(it++);
		else
			++it;
	}
}

void CategoryLoader::setUnion(cset& s1, const cset& s2)
{
	s1.insert(s2.begin(), s2.end());
}

cset CategoryLoader::xmlProcessInclude(const Item * item, menu_file_pos * mfp, QDomElement& xnode, const cset& setin, int plevel)
{
	static const auto AND = "And";
	static const auto OR = "Or";
	static const auto NOT = "Not";
	cset xset;
	QString xtag = xnode.tagName();
	QString slevel = QString().sprintf("%*s", 3 * plevel, "");
	catDebug("%sTag %s input set %s", QS(slevel), QS(xtag), QS(set2buf(setin)));

	if (xtag == "Include")
	{
		QDomNodeList xlist = xnode.childNodes();
		cset result;
		for (size_t i = 0; i < xlist.length(); i++)
		{
			QDomElement xchild = xlist.item(i).toElement();
			cset r = xmlProcessInclude(item, mfp, xchild, setin, plevel + 1);
			setUnion(result, r);
		}
		return result;
	}

	else if (xtag == AND || xtag == OR || xtag == NOT)
	{
		QDomNodeList xlist = xnode.childNodes();
		bool xin = false;
		for (size_t i = 0; i < xlist.length(); i++)
		{
			QDomElement xchild = xlist.item(i).toElement();
			QString xchildtag = xchild.tagName();

			catDebug("%si=%i child tag <%s>", QS(slevel), (int) i, QS(xchildtag));
			if (xchildtag == "")
				continue;
			cset child_set = xmlProcessInclude(item, mfp, xchild, setin, plevel + 1);
			if (!xin)
			{
				xset = child_set;
				xin = true;
			} else
			{
				if (xtag == AND)
					setIntersect(xset, child_set);
				if (xtag == OR)
					setUnion(xset, child_set);
				if (xtag == NOT)
					setUnion(xset, child_set); //Union!
			}
			if (xtag == AND && xset.empty())
				break;
		}
		if (xin && xtag == NOT)
		{
			cset s = setin;
			setSubtract(s, xset);
			xset = s;
		}
	} else if (xtag == "All")
	{
		xset = setin;
	} else if (xtag == "Filename")
	{
		QString fname = xnode.text();
		catDebug("in Filename %s == %s", QS(item->rec.fname), QS(fname));
		if (item->rec.fname.endsWith(fname))
		{
			QString parentNodeCategory = mfp->xname.text();
			if (parentNodeCategory != "")
			{
				xset.insert(parentNodeCategory);
				catDebug("%sInclude filename %s: %s", QS(slevel), QS(fname), QS(set2buf(xset)));
			}
		}

	} else if (xtag == "Category")
	{
		QString val = xnode.text();
		catDebug("%sCategory %s", QS(slevel), QS(val));
		xset.clear();
		if (setin.find(val) != setin.end())
			xset.insert(val);
	}

	catDebug("%sresult %s", QS(slevel), QS(set2buf(xset)));

	return xset;
}

void CategoryLoader::loadCategoryDirFiles()
{
	if (cacheDirByShortName.empty()) //initialize cache once
	{
		//TIMERINIT();
		Toolkit toolkit;
		vector<QString> cdirs;
		vector<QString> vdir;
		cdirs.push_back("/usr/share/mate/desktop-directories/");
		cdirs.push_back("/var/lib/menu-xdg/desktop-directories/menu-xdg/");
		cdirs.push_back("/usr/share/desktop-directories/");

		for (auto dir : cdirs)
		{
			toolkit.getDirectory(dir, &vdir);
		}

		for (auto file : vdir)
		{
			if (!file.endsWith(".directory"))
				continue;
			QString shortName = QFileInfo(file).fileName();
			cacheDirByShortName[shortName] = file;
		}
		//TIMER("dir files", t1, t2);
	}

}

QString CategoryLoader::getDirFileByShortName(QString shortDirName)
{
	auto iter = cacheDirByShortName.find(shortDirName);
	if (iter != cacheDirByShortName.end())
		return (*iter).second;
	return "";
}

ConfigMap& CategoryLoader::getCategoryDirFileMap(QString shortDirName)
{
	static ConfigMap empty;
	try
	{
		return cacheDirFileMaps.at(shortDirName);
	} catch (...)
	{
		QString dirFilePath = getDirFileByShortName(shortDirName);
		if (!dirFilePath.isEmpty())
		{
			ConfigMap tmap;
			Loader::loadMapWithFilter(dirFilePath, tmap);
			cacheDirFileMaps[shortDirName] = tmap;
			return cacheDirFileMaps.at(shortDirName);
		}
	}
	return empty;
}

void CategoryLoader::free()
{
	for (auto e : categoryFiles)
	{
		for (auto ep : e->xlist)
			delete ep;
		delete e;
	}
	categoryFiles = std::vector<menu_file*>();
	cacheDirFileMaps = std::map<QString, ConfigMap>();
	cacheDirByShortName = std::map<QString, QString>();
}
