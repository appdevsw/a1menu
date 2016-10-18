#ifndef QT_LOADER_CATEGORYLOADER_H_
#define QT_LOADER_CATEGORYLOADER_H_

#include <vector>
#include <set>
#include <map>
#include <string>
#include "configmap.h"
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include "desktoprec.h"

using std::string;

typedef std::set<string> cset;

class Item;
class ConfigMap;

class CategoryLoader
{
public:

    //do not change names , they are used inside quotes "..."
    struct CategoryRec
    {
        string category;
        string dirFile;
        string dirFilePath;
        string loctitle;
        string comment;
        string loccomment;
        string iconName;
    };

    CategoryLoader();
    virtual ~CategoryLoader();

    void init(std::vector<DesktopRec> * vapplications);
    CategoryRec getCategory(const DesktopRec& deskrec);
    void free();

private:
    std::map<string, cset> vappcat;
    std::map<string, DesktopRec*> vappmap;
    std::map<string, CategoryRec> vcatrec;

    struct MenuFilePos
    {
        string categoryName;
        xmlNodePtr xnodeInclude = NULL;
        string dirFile;
    };
    struct MenuFile
    {
        xmlDocPtr xmlDoc = NULL;
    };

    std::vector<MenuFile> vmenufiles;
    std::vector<MenuFilePos> vmenus;
    std::map<string, std::vector<string>> vdirectories;

    void findCategories(std::vector<DesktopRec> * vapplications);
    string getDesktopKey(const DesktopRec& deskrec);
    void xmlFindMenuEntries(xmlNodePtr x, int level);
    cset xmlProcessInclude(xmlNodePtr xnode, const cset& setin, int level);
    void setUnion(cset& s1, const cset& s2);
    void setIntersect(cset& s1, const cset& s2);
    void setSubtract(cset& s1, const cset& s2);

    void loadCategoryDirFiles();
    void getDirectoryData(CategoryRec& crec);

    xmlNodePtr firstChild(xmlNodePtr parent, const string& childName, int type = XML_ELEMENT_NODE);
    vector<xmlNodePtr> children(xmlNodePtr par);
    string xnodeText(xmlNodePtr par);
    string xnodeName(xmlNodePtr par);

};

#endif /* QT_LOADER_CATEGORYLOADER_H_ */
