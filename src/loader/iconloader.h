#ifndef ICONLOADER_H_
#define ICONLOADER_H_

#include <QString>
#include <QIcon>
#include <vector>
#include <map>
#include <set>

using std::vector;
using std::map;
using std::set;

class UserEvent;

class IconLoader
{
public:

	struct parse_path_data
	{
		QString symbol;
		QString theme;
		QString ext;
		int size;
		bool valid;
	};

	IconLoader();
	virtual ~IconLoader();
	QIcon getIcon(QString iname, int isize, QString theme = "");
	QString getIconPath(QString iname, int isize, QString theme = "");
	void free();
	void parseIconParameter(QString iname, QString& symbol, QString& theme);
	void parseIconPath(const QString& path, parse_path_data& rec);
	QPixmap getPixmap(QString path, int isize);
	QString normPath(QString path);
	QString getCachePath(QString path, int isize);
	void setCachingInSeparateProcess(bool enable);
	int saveIndex(QString fname);
	int loadIndex(QString fname);

	QString defaultIconName;

private:

	struct vpathdict_rec
	{
		QString path;
		QString ext;
	};

	QString getIconPathPrv(QString symbol, int isize, QString theme);
	vector<int> getPreferredSizes(int size);
	void init();
	bool isBetterSize(int reqSize, int currSize, int newSize);
	void fillIndex(const QString& path);

	QString xdgIconPath;
	QStringList xdgIconPathSplit;

	vector<int> visizesorg { 48, 32, 24, 22, 16 }; //order matters
	vector<QString> vext = { "png", "xpm", "svg" };
	vector<QString> vextdot = { ".png", ".xpm", ".svg" };
	vector<QString> vthemes;
	set<QString> vextset = set<QString>(vext.begin(), vext.end());
	set<int> vsizesset = set<int>(visizesorg.begin(), visizesorg.end());

	map<QString, int> vthemesmap;
	map<int, vpathdict_rec> vpathdict;
	map<QString, vector<unsigned short>> vindex;
	bool indexLoaded = false;
	int indexCount=0;
	bool cachingInProcess=true;
	QString cacheFileName;

};

#endif /* ICONLOADER_H_ */
