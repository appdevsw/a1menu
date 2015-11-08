#ifndef LOADER_H
#define LOADER_H

#include <QString>
#include "iconloader.h"
#include "categoryloader.h"
#include "configmap.h"
#include <queue>
#include <vector>
#include <map>
#include <set>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <fstream>
#include "ctx.h"

using std::queue;
using std::vector;
using std::map;
using std::set;

class QProcess;
class UserEvent;
class Item;
class ImgConv;

class Loader
{
public:

	struct lcons
	{
		QString fenv = PATH(CACHE)+"loaderenv.txt";
		QString fitems = PATH(CACHE)+"loaderitems.txt";
		QString fcategories = PATH(CACHE)+"loadercategories.txt";
		QString ficons = PATH(CACHE)+"loadericons.txt";
		QString fctl = PATH(CACHE)+"loaderctl.txt";
	};

	static lcons str;


	Loader();
	~Loader();

	void initServer();
	void srvMain();
	int cliLoadItems(bool nowait=false);
	int cliLoadCategories(bool nowait=false);
	int cliLoadIcons(bool nowait=false);
	int cliLoadEnv(bool nowait=false);
	void closeServer();
	void onLoaderEvent(UserEvent * ue);
	static void loadMapWithFilter(QString fname,ConfigMap& cmap);

private:

	struct
	{
		bool env=false;
		bool items=false;
		bool categories=false;
		bool icons=false;
	} progress;

	struct catrec
	{
		Item * icat;
		ConfigMap cans;
	};



	void pack(ConfigMap& cmap,QString& buf);
	void unpack(const QString& msg,ConfigMap& cmap );
	std::string encode(const QString& msg);
	QString decode(const char * line);

	void srvSend(std::ofstream& fout,ConfigMap& cmap);
	void srvRequestIcon(std::ofstream& fout,QString subtype, QString ptr,QString symbol, int size, QString theme);
	void srvLoadItems();
	void srvInitEnv();
	void srvLoadCategories();
	void srvLoadIcons();
	void srvRemoveDuplicates(vector<Item*>& vitems);
	catrec srvCreateCategory(QString catName,CategoryLoader::category_rec& crec,ConfigMap& cans);


	int cliProcessAnswerFile(QString fname,bool nowait=false);
	int cliAnswerIcon(ConfigMap& cmap);
	int cliAnswerApp(ConfigMap& cmap);
	int cliAnswerCategory(ConfigMap& cmap);
	int cliAnswerHidden(ConfigMap& cmap);
	void cliPostActions();

	void clearLoaderFiles();
	void createCtlFile();
	bool checkCtlFile();

	QProcess * process = NULL;
	int srvpid=0;
	IconLoader iconLoader;
	CategoryLoader categoryLoader;
	vector<Item *> vitems;
	vector<Item *> vcategories;
	map<int,Item *> vserials;
	map<int, vector<int>> vseriallist;
	map<QString,long long> vwait;

	int itemSerial=0;
	ImgConv * imgconv=NULL;

	std::map<QString, catrec> vcatmapbyname;
	std::map<int, catrec> vcatmapbyserial;

};

#endif // LOADER_H
