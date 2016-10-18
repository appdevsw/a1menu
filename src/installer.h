#ifndef A1MENU_GTK_SRC_INSTALLER_H_
#define A1MENU_GTK_SRC_INSTALLER_H_

#include <string>
#include <vector>

class Installer
{
public:


    struct Entry
    {
        string content;
        string srcPath;
        string fileName;
        string tmpFileName;
        bool md5 = true;
    };

    Installer();
    virtual ~Installer();
    void getEntries(std::vector<Entry> & ventries);
    int createDebPackage(std::string targetDir);
    int createRpmPackage(std::string targetDir);


private:
    string currentWorkingDir;
    string exePath;
    string gtkVersionSym;
    string appVersion;
    string architecture;
    string tmpRoot;

    void syscmd(string cmd);
    int createEntryFile(Entry& e);
    bool checkUtilExists(string cmd,string errmsg);
};

#endif /* A1MENU_GTK_SRC_INSTALLER_H_ */
