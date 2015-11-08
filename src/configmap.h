#ifndef CONFIGMAP_H
#define CONFIGMAP_H

#include <vector>
#include <map>
#include <QString>

class ConfigMap
{

public:
	struct entry
	{
		QString value;
	};
	ConfigMap();
	void clear();
	int load(QString fname, bool override = false);
	int save(QString fname,bool skipEmpty=false);
	QString& operator[](const QString& key);
	void setCheckExistence(bool c);
	void setKeyCaseSensitive(bool c);
	void puti(QString key, int val);
	int split(QString key, std::vector<QString>& arr);
	void copyTo(ConfigMap& c);
	std::map<QString, QString> getMap();
	const std::map<QString, ConfigMap::entry>& getMapRef();
private:
	QString listSep = ";";
	std::map<QString, entry> mapKV;
	bool caseSensitive = false;
	bool doCheck = false;

	inline QString index(const QString& key);
	inline entry& getEntry(const QString& key);
	void put(const QString& key,const  QString& val);
	QString get(const QString& key);

};

#endif // CONFIGMAP_H
