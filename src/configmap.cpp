#include "configmap.h"
#include <algorithm>
#include <string>
#include <sstream>
#include <fstream>
#include "toolkit.h"
#include "sutl.h"

using namespace std;

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

inline string ConfigMap::index(const string& key)
{
    if (caseSensitive)
        return key;
    string low = key;
    std::transform(low.begin(), low.end(), low.begin(), ::tolower);
    return low;
}

int ConfigMap::load(const string& fname, bool override)
{
    Toolkit toolkit;
    vector<string> vlines;
    int res = toolkit.getFileLines(fname, vlines);
    if (res)
        return res;
    if (override)
        clear();
    for (string line : vlines)
    {
        size_t pos = line.find("=");
        if (pos != string::npos)
        {
            string key = index(line.substr(0, pos));
            string val = line.substr(pos + 1);
            Entry * e = getEntry(key);
            e->value = val;
        }
    }
    return 0;
}

int ConfigMap::save(const string& fname)
{
    auto it = mapKV.begin();
    std::ofstream ofile(fname);
    while (it != mapKV.end())
    {
        string line = index(it->first) + "=" + (it->second.value) + "\n";
        ofile << line;
        it++;
    }
    ofile.close();
    return 0;
}

std::map<string, string> ConfigMap::getMap()
{
    std::map<string, string> m;
    for (auto e : mapKV)
        m[e.first] = e.second.value;
    return m;
}

ConfigMap::Entry * ConfigMap::getEntry(const string& key)
{
    auto it = mapKV.find(index(key));
    if (it == mapKV.end())
    {
        if (doCheck)
        {
            printf("\nWrong configuration parameter %s", key.c_str());
            exit(1);
        }
        Entry e = {};
        mapKV[index(key)] = e;
    }
    return &(mapKV[index(key)]);
}

bool ConfigMap::exists(const string& key)
{
    return mapKV.find(index(key)) != mapKV.end();

}

bool ConfigMap::remove(const string& key)
{
    auto it = mapKV.find(index(key));
    auto res = it != mapKV.end();
    if (res)
        mapKV.erase(it);
    return res;
}

string ConfigMap::get(const string& key)
{
    return (*getEntry(key)).value;
}
int ConfigMap::geti(const string& key)
{
    auto v = (*getEntry(key)).value;
    return atoi(v.c_str());
}

string& ConfigMap::operator[](const string& key)
{
    return (*getEntry(key)).value;
}

void ConfigMap::put(const string& key, const string& val)
{
    (*getEntry(key)).value = val;

}

void ConfigMap::puti(const string& key, int val)
{
    std::ostringstream stm;
    stm << val;
    (*getEntry(key)).value = stm.str();
}

int ConfigMap::split(const string& key, std::vector<string>& arr)
{
    arr.clear();
    vector<string> xarr;
    string val = get(key);
    sutl::tokenize(val, xarr, listSep);
    for (auto s : xarr)
    {
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
        Entry e = it->second;
        *(c.getEntry(it->first)) = e;
        it++;
    }

}

string ConfigMap::pack()
{
    string msg;
    int count = 0;
    auto it = mapKV.begin();
    while (it != mapKV.end())
    {
        if (count++ > 0)
            msg += "\n";
        msg += it->first;
        msg += "=";
        msg += it->second.value;
        it++;
    }
    return msg;
}

void ConfigMap::unpack(const string& packed)
{
    clear();
    auto v = sutl::split(packed, '\n');
    for (auto line : v)
    {
        size_t pos = sutl::indexOf(line, "=");
        if (pos >= 0)
        {
            auto v = line.substr(pos + 1);
            put(line.substr(0, pos), v);
        }
    }
}
