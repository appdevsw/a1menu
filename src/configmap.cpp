#include "configmap.h"
#include "ctx.h"
#include "toolkit.h"
#include <QIODevice>
#include <QFile>
#include <QTextStream>

using namespace std;
using namespace ctx;

ConfigMap::ConfigMap()
{
}

void ConfigMap::clear()
{
	mapKV.clear();
}

void ConfigMap::setCheckExistence(bool c)
{
	doCheck = c;
}

void ConfigMap::setKeyCaseSensitive(bool c)
{
	caseSensitive = c;
}

inline QString ConfigMap::index(const QString& key)
{
	return (caseSensitive ? key : key.toLower()).trimmed();
}

inline ConfigMap::entry& ConfigMap::getEntry(const QString& key)
{
	return mapKV.insert(std::pair<QString, entry>(index(key), {})).first->second;
	/*
	 auto ikey = index(key);
	 auto it = mapKV.find(ikey);
	 if (it != mapKV.end())
	 return it->second;
	 if (doCheck)
	 {
	 //qDebug("Config: wrong parameter <%s>", QS(key));
	 ctx::errorDialog("Wrong configuration parameter " + key);
	 exit(1);
	 }
	 mapKV[ikey];
	 return mapKV.at(ikey);
	 */
}

QString ConfigMap::get(const QString& key)
{
	return getEntry(key).value;
}

QString& ConfigMap::operator[](const QString& key)
{
	return getEntry(key).value;
}

void ConfigMap::put(const QString& key, const QString& val)
{
	getEntry(key).value = val.trimmed();

}

void ConfigMap::puti(QString key, int val)
{
	getEntry(key).value = QString::number(val);

}

int ConfigMap::load(QString fname, bool override)
{
	vector<QString> vlines;
	int res = Toolkit().getFileLines(fname, vlines);
	if (res)
		return res;
	if (override)
		clear();
	for (QString line : vlines)
	{
		size_t pos = line.indexOf("=");
		if (pos >= 0)
		{
			QString key = index(line.mid(0, pos).trimmed());
			QString val = line.mid(pos + 1).trimmed();
			entry& e = getEntry(key);
			e.value = val;
		}
	}

	return 0;
}

int ConfigMap::save(QString fname, bool skipEmpty)
{
	QFile outputFile(fname);
	outputFile.open(QIODevice::WriteOnly);
	if (!outputFile.isOpen())
	{
		qDebug("Error, unable to open %s for output", QS(fname));
		return -1;
		//exit(1);
	}
	QTextStream outStream(&outputFile);
	for (auto it = mapKV.begin(); it != mapKV.end(); it++)
	{
		if (skipEmpty && it->second.value.isEmpty())
			continue;
		outStream << index(it->first) << "=" << (it->second.value) << "\n";
	}
	outputFile.close();
	return 0;
}

int ConfigMap::split(QString key, std::vector<QString>& arr)
{
	arr.clear();
	vector<QString> xarr;
	QString val = get(key);
	Toolkit().tokenize(val, xarr, listSep);
	for (auto s : xarr)
	{
		s = s.trimmed();
		if (s != "")
			arr.push_back(s);
	}
	return arr.size();
}

void ConfigMap::copyTo(ConfigMap& c)
{
	c.clear();
	auto it = mapKV.begin();
	while (it != mapKV.end())
	{
		entry e = it->second;
		//c[e.name] = it->first;
		c.getEntry(it->first) = e;
		it++;
	}

}

std::map<QString, QString> ConfigMap::getMap()
{
	std::map<QString, QString> m;
	for (auto e : mapKV)
		m[e.first] = e.second.value;
	return m;
}

const std::map<QString, ConfigMap::entry>& ConfigMap::getMapRef()
{
	return mapKV;
}
