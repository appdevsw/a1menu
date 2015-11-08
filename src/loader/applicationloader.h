#ifndef MENUDIR_H
#define MENUDIR_H

#include <vector>
#include <set>
#include <map>
#include <QString>

class ConfigMap;
class Item;

class ApplicationLoader
{
public:
	ApplicationLoader();
	virtual ~ApplicationLoader();
	void getItems(std::vector<Item*> & arr);
	void init();
	static QString getMapLocValue(ConfigMap& cmap,QString name);
	void insertPlaces(std::vector<Item *>& itemarr);

private:
	void parseFile(QString& fname, std::vector<Item*> & arr);
	Item * parseFileEntry(ConfigMap& cmap, std::vector<Item*> & arr,QString& fname);
	int checkUnique(ConfigMap& cmap,QString& fname);
	QString desktopPath;
};

#endif // MENUDIR_H
