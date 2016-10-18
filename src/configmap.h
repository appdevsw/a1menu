#ifndef CONFIGMAP_H
#define CONFIGMAP_H

#include <vector>
#include <map>
#include <string>

using std::string;
using std::vector;
using std::map;

class ConfigMap
{
public:
	struct Entry
	{
		string value;
	};
	ConfigMap();
	void clear();
	int load(const string& fname, bool override = false);
	int save(const string& fname);
	string& operator[](const string& key);
	void setCheckExistence(bool c);
	void setKeyCaseSensitive(bool c);
	void puti(const string& key, int val);
    int  geti(const string& key);
	int split(const string& key, std::vector<string>& arr);
	void copyTo(ConfigMap& c);
	Entry * getEntry(const string& key);
	string pack();
	void unpack(const string& packed);
    std::map<string, string> getMap();
    bool exists(const string& key);
    bool remove(const string& key);

private:
	string listSep = ";";
	std::map<string, Entry> mapKV;
	bool caseSensitive = false;
	bool doCheck = false;

	inline string index(const string& key);
	void put(const string& key, const string& val);
	string get(const string& key);

};


#endif // CONFIGMAP_H
