#ifndef TOOLKIT_H
#define TOOLKIT_H
#include <string>
#include <vector>
#include <map>

using std::string;
using std::vector;

class Toolkit
{
public:
    Toolkit();
    virtual ~Toolkit();
    int getDirectory(const string& dirName, vector<string> * arr, const string& ext = "");
    string shortName(const string& s);
    string suffix(const string& s);
    string homeDir();
    string desktopDir();
    string md5(const string&);
    string fileMD5(const string& fname);
    bool fileExists(const string& fname);
    bool dirExists(const string& pathname);
    void load(const string& fname, string& buf);
    int save(const string& fname, const string& buf);
    int getFileLines(const string& fname, vector<string>& vlines);
    typedef unsigned long long microsectype;
    microsectype GetMicrosecondsTime();

};

/*
 class FDebug
 {
 private:
 char buf[1024];
 public:
 FDebug(string fname);
 void prnf(char * buf);
 void prn(const char* format, ...);
 void clear();
 };*/

#endif // TOOLKIT_H
