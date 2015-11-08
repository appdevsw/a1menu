#include "iconloader.h"
#include "categoryloader.h"
#include "toolkit.h"
#include "configmap.h"
#include <assert.h>
#include <iostream>
#include "ctx.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <fstream>
#include <unistd.h>
#include <dirent.h>
#include <imgconv.h>
#include <QImageReader>
#include <QPainter>
#include <QDirIterator>
#include <QDialog>
#include <QThread>
#include "searchbox.h"
#include "userevent.h"
#include "mainwindow.h"
#include "toolbutton.h"
#include "itemlist.h"
#include "itemproperties.h"
#include <QTextStream>
#include <string>
#include <zlib.h>
#include <errno.h>
#include <sstream>
#include <QStringBuilder>

using std::vector;
using std::map;
using std::set;
using namespace ctx;

IconLoader::IconLoader()
{
	cacheFileName = PATH(CACHE)+ "iconcache.txt";
}

IconLoader::~IconLoader()
{
}

void IconLoader::free()
{
	indexLoaded = false;
	vthemesmap = map<QString, int>();
	vthemes = vector<QString>();
	vpathdict = map<int, vpathdict_rec>();
	vindex = map<QString, vector<unsigned short>>();
	//qDebug("icon loader free");

}

void IconLoader::init()
{
	if (!indexLoaded)
	{
		indexCount = 0;
		xdgIconPath = normPath(PATH(XDG_ICON_PATH));
		xdgIconPathSplit = xdgIconPath.split("/", QString::SkipEmptyParts);

		indexLoaded = true;
		vthemes.clear();

		QString mainThemes[] = { "mate", "hicolor", "matefaenza", "oxygen" };
		CFGPTR()->split("icons_preferred_themes", vthemes);
		for (auto mt : mainThemes)
		{
			if (std::find(vthemes.begin(), vthemes.end(), mt) != vthemes.end())
				continue;
			vthemes.push_back(mt);
		}
		vthemes.push_back("");

		int themeId = 0;
		for (auto t : vthemes)
		{
			vthemesmap[t] = themeId++;
			//qDebug("theme %s index %i", QS(t), vthemesmap[t]);
		}

		ctx::clockmicro_t begin = ctx::clockmicro();

		std::vector<QString> vaddpaths;
		CFGPTR()->split("icons_additional_paths", vaddpaths);

		//-------------------------------
		int useIndexFile = 0;
		//------
		bool loaded = false;
		if (useIndexFile && Toolkit().fexists(cacheFileName))
		{
			qDebug("icons from cache file");
			if (loadIndex(cacheFileName) == 0)
				loaded = true;
		}

		if (!loaded)
		{
			for (QString apath : vaddpaths)
				fillIndex(apath);

			fillIndex(QDir::homePath() + "/.icons");
			fillIndex(QDir::homePath() + "/.local/share/icons");
			fillIndex(xdgIconPath);
			fillIndex("/usr/share/pixmaps");
			if (useIndexFile)
				saveIndex(cacheFileName);
		}

		double el = double(ctx::clockmicro() - begin) / 1000.0;

		qDebug("icon loader indexing time:  %9.0lf ms, icon count %i", el, indexCount);
	}

}

int IconLoader::saveIndex(QString fname)
{
	gzFile fzout = gzopen(QS(fname), "wb");
	if (fzout == NULL)
	{
		fprintf(stderr, "gzopen: %s: %s\n", QS(fname), strerror(errno));
		return -1;
	}

	QString sep = "*";
	for (auto e : vpathdict)
	{
		QString line = QString("p") % sep % QString::number(e.first) % sep % e.second.ext % sep % e.second.path % "\n";
		gzwrite(fzout, QS(line), line.length());
	}
	for (auto e : vindex)
	{
		QString line = "i" % sep % e.first % sep;
		for (int pathref : e.second)
		{
			line = line % QString::number(pathref) % ";";
		}
		line = line % "\n";
		gzwrite(fzout, QS(line), line.length());
	}
	gzclose(fzout);
	return 0;

}

int IconLoader::loadIndex(QString fname)
{
	gzFile fzin = gzopen(QS(fname), "rb");
	if (fzin == NULL)
	{
		perror("gzdopen");
		return -1;
	}
	QString sep = "*";

	vpathdict.clear();
	vindex.clear();

	char buf[1024];
	QString bufline;
	for (;;)
	{
		int n = gzread(fzin, buf, sizeof(buf));
		if (n <= 0)
		{
			if (bufline.isEmpty())
				break;
			else
				bufline = bufline % "\n";
		}
		bufline = bufline % QString::fromLocal8Bit(buf, n);

		int p1 = 0;
		int p2 = bufline.indexOf("\n");
		while (p2 >= 0)
		{

			QString line = bufline.mid(p1, p2 - p1);
			p1 = p2 + 1;
			p2 = bufline.indexOf("\n", p1);
			if (p2 < 0)
				bufline = bufline.mid(p1);

			//----------- line is ready here
			{
				//qDebug("%s",QS(line));
				auto spl = line.split(sep);
				if (spl[0] == "p")
				{
					vpathdict_rec rec = { spl[3], spl[2] };
					vpathdict[spl[1].toInt()] = rec;
				}
				if (spl[0] == "i")
				{
					auto indexes = spl[2].split(";", QString::SkipEmptyParts);
					auto& vref = vindex.insert(std::pair<QString, vector<unsigned short>>(spl[1], {})).first->second;
					for (auto idx : indexes)
					{
						vref.push_back(idx.toInt());
						indexCount++;
					}
				}

			}
			//--------------
		}
	}

	gzclose(fzin);
	return 0;

}

void IconLoader::parseIconParameter(QString iname, QString& symbol, QString& theme)
{
	Toolkit toolkit;
	vector<QString> v;
	toolkit.tokenize(iname, v, ";");
	if (v.empty())
	{
		symbol = theme = "";
		return;
	}
	theme = v.size() > 1 ? v[1] : "";
	symbol = v[0];
}

QString IconLoader::normPath(QString path)
{
	QString p = path + "/";
	p.replace("//", "/");
	return p;
}

void IconLoader::parseIconPath(const QString& path, parse_path_data& rec)
{
	rec.ext = rec.symbol = rec.theme = "";
	rec.size = 0;
	bool isXdgPath = path.startsWith(xdgIconPath);
	auto spl = path.split("/", QString::SkipEmptyParts);
	//qDebug("parse  %s",QS(path));
	for (int i = 0; i < spl.length(); i++)
	{
		auto word = spl[i];
		//qDebug(" word <%s>",QS(word));
		if (word.at(0).isDigit())
		{
			QString size;
			for (QString::const_iterator itr(word.begin()); itr != word.end(); ++itr)
			{
				auto c = *itr;
				if (!c.isDigit())
					break;
				size += c;
			}
			if (size != "" && (size == word || size + "x" + size == word))
				rec.size = size.toInt();
		}
		if (isXdgPath && i == xdgIconPathSplit.length())
			rec.theme = word;
		if (i + 1 == spl.length())
		{
			int p = word.lastIndexOf(".");
			if (p >= 0)
			{
				rec.ext = word.mid(p + 1);
				rec.symbol = word.mid(0, p);
			} else
			{
				rec.symbol = word;
			}
		}
	}
	rec.valid = path.startsWith("/") && rec.ext != "" && rec.symbol != "";

	//qDebug("%-20.20s %5i %-30.30s %7s   %s", QS(rec.theme), rec.size, QS(rec.symbol), QS(rec.ext), QS(path));
}

void IconLoader::fillIndex(const QString& pipath)
{

	DIR *dir;
	struct dirent *ent;
	map<QString, int> pathsext;
	QString ipath = normPath(pipath);
	if ((dir = opendir(QS(ipath))) == NULL)
	{
		//perror(QS(ipath));
		return;
	}
	while ((ent = readdir(dir)) != NULL)
	{
		QString fname = ent->d_name;
		parse_path_data rec;
		if (fname == "." || fname == "..")
			continue;
		QString childPath = ipath % fname;
		if (ent->d_type == DT_DIR)
		{
			parseIconPath(childPath, rec);
			//if (rec.size <= 0 || (rec.size > 0 && vsizesset.find(rec.size) != vsizesset.end()))
			if (rec.theme == "" || vthemesmap.find(rec.theme) != vthemesmap.end())
				fillIndex(childPath);

		} else
		{
			int p = fname.lastIndexOf(".");
			if (p >= 0)
			{
				rec.symbol = fname.mid(0, p);
				rec.ext = fname.mid(p + 1);
			} else
			{
				rec.symbol = fname;
				rec.ext = "";
			}
			//qDebug("ext %s %s",QS(rec.ext),QS(childPath));
			if (vextset.find(rec.ext) == vextset.end())
				continue;
			unsigned short pathID;
			if (pathsext.find(rec.ext) == pathsext.end())
			{
				pathID = vpathdict.size();
				vpathdict[pathID] =
				{	ipath, rec.ext};
				pathsext[rec.ext] = pathID;
			} else
				pathID = pathsext[rec.ext];
			//qDebug("path is %i %s",pathID,QS(ipath));

			//construct the vector (once) and add
			vindex[rec.symbol].push_back(pathID);
			indexCount++;
		}
	}
	closedir(dir);
}

bool IconLoader::isBetterSize(int reqSize, int currSize, int newSize)
{
	if (currSize <= 0)
		return true;
	if (currSize == reqSize)
		return false;
	if (newSize == reqSize)
		return currSize != newSize;
	if (newSize == 2 * reqSize and newSize != currSize)
		return true;
	if (newSize == (3 * reqSize) / 2 and newSize != currSize)
		return true;
	if (newSize > reqSize and newSize < currSize)
		return true;
	if (currSize < reqSize and newSize > currSize)
		return true;
	return false;

}

QString IconLoader::getIconPathPrv(QString iname, int isize, QString theme)
{
	assert(isize != 0);
	if (iname.indexOf(".") >= 0)
		for (auto ext : vextdot)
		{
			if (iname.endsWith(ext))
			{
				iname = iname.mid(0, iname.length() - ext.length());
				break;
			}
		}
	init();
	if (vindex.find(iname) == vindex.end())
		return "";
	if (theme == "")
		theme = vthemes[0];
	if (isize <= 0)
		isize = visizesorg[0];

//qDebug("get path for %s  %s %i", QS(iname), QS(theme), isize);

	QString pathTheme;
	int pathThemeSize = -1;

	QString pathAnyTheme;
	int pathAnyThemeSize = -1;
	int anyThemeIdx = 99999;
	QString res = "";

	for (auto pathID : vindex[iname])
	{
		vpathdict_rec prec = vpathdict[pathID];
		QString ipath = prec.path + iname + "." + prec.ext;
//qDebug(" %s", QS(ipath));
		parse_path_data rec;
		parseIconPath(ipath, rec);
		if (theme == rec.theme && isize == rec.size)
		{
			res = ipath;
			break;
		}

		if (theme == rec.theme && isBetterSize(isize, pathThemeSize, rec.size))
		{
			pathTheme = ipath;
			pathThemeSize = rec.size;
			//qDebug(" better1 %s", QS(ipath));
		}

		int newIdx = vthemesmap.find(rec.theme) == vthemesmap.end() ? 9999 : vthemesmap[rec.theme];
		if (newIdx < anyThemeIdx)
		{
			pathAnyThemeSize = -1;
		}

		if (newIdx <= anyThemeIdx)
		{
			//qDebug(" newIdx %i anyThemeIdx %i",newIdx,anyThemeIdx);
			if (isBetterSize(isize, pathAnyThemeSize, rec.size))
			{
				//qDebug(" better2 %s", QS(ipath));
				pathAnyTheme = ipath;
				pathAnyThemeSize = rec.size;
				anyThemeIdx = newIdx;
			}
		}

	}
	if (res == "")
		res = pathTheme;
	if (res == "")
		res = pathAnyTheme;
	//qDebug(" finally %s", QS(res));

	return res;
}

QString IconLoader::getIconPath(QString iname, int isize, QString theme)
{
	//assert(iname != "/");

	QString symbol, itheme;
	parseIconParameter(iname, symbol, itheme);
	if (theme.isEmpty())
		theme = itheme;

	if (symbol.contains("/"))
	{
		parse_path_data rec;
		parseIconPath(symbol, rec);
		if (rec.valid)
			return symbol;
		return "";
	}
	if (symbol.isEmpty())
		return "";
	QString ipath = getIconPathPrv(symbol, isize, theme);

	return ipath;
}

QIcon IconLoader::getIcon(QString iname, int isize, QString theme)
{

	QString symbol, itheme;
	parseIconParameter(iname, symbol, itheme);
	if (theme.isEmpty())
		theme = itheme;
	QString path = getIconPath(symbol, isize, theme);
	path = getCachePath(path, isize);
	return QIcon(path);
}

void IconLoader::setCachingInSeparateProcess(bool enable)
{
	cachingInProcess = enable;
}

QString IconLoader::getCachePath(QString path, int isize)
{
	if (path == "")
		return path;
	if (path.startsWith(PATH(CACHE)))
		return path;

	IconLoader::parse_path_data irec;
	parseIconPath(path, irec);
	if (irec.ext == "svg" || irec.size != isize)
	{
		QString ipath;
		auto mode = cachingInProcess ? ImgConv::SEPARATE_PROCESS : ImgConv::SAME_PROCESS;
		ipath = ImgConv().getPngFromCache(path, isize, mode);
		if (ipath != "")
			return ipath;
		return "";
	}
	return path;
}

QPixmap IconLoader::getPixmap(QString path, int isize)
{
	if (path == "")
		return QPixmap();

	IconLoader::parse_path_data irec;
	parseIconPath(path, irec);
	if (irec.valid)
	{
		QPixmap pix;
		QString ipath = getCachePath(path, isize);
		if (ipath != "")
		{
			assert(ipath.contains("/"));
			QIcon icon = QIcon(ipath);
			int isp = isize;
			if (irec.size > 0)
				isp = irec.size;
			pix = icon.pixmap(isp, isp);
		}
		return pix;
	}
	return QPixmap();
}

