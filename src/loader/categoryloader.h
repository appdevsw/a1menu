#ifndef QT_LOADER_CATEGORYLOADER_H_
#define QT_LOADER_CATEGORYLOADER_H_

#include <QDomElement>
#include <QDomDocument>
#include <vector>
#include <set>
#include <map>


typedef std::set<QString> cset;

class Item;
class ConfigMap;

class CategoryLoader
{
friend class Loader;
public:

	struct category_rec
	{
		QString category;
		QString dirFile;
		QString dirFilePath;
	};

	CategoryLoader();
	virtual ~CategoryLoader();

	void init();
	void findCategory(Item * item, category_rec& rec);
	QString getDirFileByShortName(QString shortDirName);
	ConfigMap& getCategoryDirFileMap(QString fname);
	void free();

private:

	category_rec desktop;
	struct menu_file_pos
	{
		QDomElement xname;
		QDomElement xinclude;
		QString    dirFile;
		cset resultset;
	};
	struct menu_file
	{
		QString fname;
		std::vector<menu_file_pos*> xlist;
	};

	struct dir_cache_t
	{
		std::set<QString> names;
		std::set<QString> files;
		QString path;
	};

	std::vector<menu_file*> categoryFiles;
	std::map<QString,QString> cacheDirByShortName;
	std::map<QString,ConfigMap> cacheDirFileMaps;

	void xmlFindMenuEntries(menu_file& mf, QDomElement& x, int level);
	cset xmlProcessInclude(const Item * item,menu_file_pos * mfp,QDomElement& xnode, const cset& setin, int level);
	QString vectorToString(std::vector<QString>&);
	void setUnion(cset& s1, const cset& s2);
	void setIntersect(cset& s1, const cset& s2);
	void setSubtract(cset& s1, const cset& s2);

	void loadCategoryDirFiles();

};

#endif /* QT_LOADER_CATEGORYLOADER_H_ */
