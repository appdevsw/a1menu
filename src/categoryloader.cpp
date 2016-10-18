#include "application.h"
#include "categoryloader.h"
#include "applicationloader.h"
#include "configmap.h"
#include "toolkit.h"
#include "sutl.h"
#include <assert.h>
#include <sstream>
#include <fstream>

using namespace std;

#define CATDBG 0

#define dbg if(CATDBG) printf

CategoryLoader::CategoryLoader()
{
}

CategoryLoader::~CategoryLoader()
{
    free();
}

xmlNodePtr CategoryLoader::firstChild(xmlNodePtr parent, const string& childName, int type)
{
    assert(parent!=NULL);
    auto n = parent->children;
    while (n != NULL)
    {
        if ((type == 0 || n->type == type) && (childName.empty() || xnodeName(n) == childName))
            return n;
        n = n->next;
    }
    return NULL;
}

vector<xmlNodePtr> CategoryLoader::children(xmlNodePtr parent)
{
    assert(parent!=NULL);
    vector<xmlNodePtr> v;
    auto n = parent->children;
    while (n != NULL)
    {
        if (n->type == XML_ELEMENT_NODE)
            v.push_back(n);
        n = n->next;
    }
    return v;
}

string CategoryLoader::xnodeText(xmlNodePtr par)
{
    assert(par!=NULL);
    auto n = firstChild(par, "text", XML_TEXT_NODE);
    if (n != NULL)
        return string((char*) n->content);
    return "";
}

string CategoryLoader::xnodeName(xmlNodePtr par)
{
    assert(par!=NULL);
    return string((char*) par->name);
}

void CategoryLoader::xmlFindMenuEntries(xmlNodePtr xnode, int level)
{
    if (xnodeName(xnode) == "Menu")
    {
        MenuFilePos mfp;
        auto xn = firstChild(xnode, "Name");
        if (xn)
            mfp.categoryName = xnodeText(xn);
        mfp.xnodeInclude = firstChild(xnode, "Include");
        xn = firstChild(xnode, "Directory");
        if (xn != NULL)
            mfp.dirFile = xnodeText(xn);

        if (!mfp.categoryName.empty() && mfp.xnodeInclude != NULL)
            vmenus.push_back(mfp);

        auto xchildren = children(xnode);
        for (int i = 0; i < (int) xchildren.size(); i++)
        {
            if (xnodeName(xchildren[i]) == "Menu")
                xmlFindMenuEntries(xchildren[i], level + 1);
        }
    }
}

void CategoryLoader::init(std::vector<DesktopRec> * vapplications)
{

    Toolkit toolkit;
    vector<string> vfiles;
    vector<string> adddirs;
    string base = "/etc/xdg/menus/";

    vfiles.push_back(base + "mate-applications.menu");
    vfiles.push_back(base + "mate-settings.menu");
    vfiles.push_back(toolkit.homeDir() + "/.config/menus/mate-applications.menu");

    vector<string> addfiles;

    for (string fname : vfiles)
    {
        if (!toolkit.fileExists(fname))
            continue;
        MenuFile mf;
        mf.xmlDoc = xmlParseFile(fname.c_str());
        if (mf.xmlDoc == NULL)
        {
            printf("\nfindCategoryName xml parser error! %s", fname.c_str());
            continue;
        }
        auto root = xmlDocGetRootElement(mf.xmlDoc);
        assert(root!=NULL);
        string rootname = xnodeName(root);
        if (rootname != "Menu")
        {
            printf("\nxml: tag `Menu` not found. %s", fname.c_str());
            continue;
        }

        xmlFindMenuEntries(root, 0);
        vmenufiles.push_back(mf);
    }

    findCategories(vapplications);
}

string set2buf(const cset& v)
{
    string str = "[";
    int count = 0;
    for (auto e : v)
    {
        if (count++ > 0)
            str += ";";
        str += e.c_str();
    }
    str += "]";
    return str;
}

CategoryLoader::CategoryRec CategoryLoader::getCategory(const DesktopRec& deskrec)
{
    string key = getDesktopKey(deskrec);
    auto it = vappcat.find(key);
    if (it != vappcat.end())
    {
        for (auto e : (*it).second)
        {
            return vcatrec[e.c_str()];
        }
    }
    dbg("\ncategory not found for: %s", deskrec.fname.c_str());
    return CategoryRec();
}

string CategoryLoader::getDesktopKey(const DesktopRec& deskrec)
{
    if (CATDBG)
    {
        string key = deskrec.fname;
        key = sutl::replace(key, " /usr/share/applications/", "");
        key = sutl::replace(key, ".desktop", "");
        return key;
    }
    return sutl::itoa(deskrec.key);
}

void CategoryLoader::findCategories(std::vector<DesktopRec> * v)
{
    vcatrec.clear();
    vappcat.clear();
    vappmap.clear();
    loadCategoryDirFiles();
    string matecckey;
    for (size_t i = 0; i < v->size(); i++)
    {
        DesktopRec & deskrec = v->at(i);
        deskrec.key = i + 1;
        string key = getDesktopKey(deskrec);
        assert(vappmap.find(key) == vappmap.end());
        vappmap[key] = &deskrec;
        if (sutl::endsWith(deskrec.fname, "matecc.desktop"))
            matecckey = key;
    }
    cset setin;
    for (auto e : vappmap)
    {
        setin.insert(e.first);
    }

    for (int i = 0; i <= 1; i++) //skip Other category first
        for (MenuFilePos mfp : vmenus)
        {
            string catname = mfp.categoryName;
            bool other = catname == "Other";
            if ((other && i == 0) || (!other && i != 0))
            {
                continue;
            }

            dbg("\n\n%s\n", catname.c_str());
            cset rset = xmlProcessInclude(mfp.xnodeInclude, setin, 0);

            for (auto key : rset)
            {
                if (other && vappcat.find(key) != vappcat.end())
                    continue;

                if (vcatrec.find(catname) == vcatrec.end() //
                || (vcatrec[catname].dirFile.empty() && !mfp.dirFile.empty()))
                {
                    CategoryRec crec;
                    crec.category = catname;
                    crec.dirFile = mfp.dirFile;
                    getDirectoryData(crec);
                    vcatrec[catname] = crec;
                    dbg("\ninsert category %s %s %s", catname.c_str(), mfp.dirFile.c_str(), crec.dirFilePath.c_str());
                }
                vappcat[key].insert(catname);
            }

        }

    //move the Control Center to Preferences
    string pref = "Preferences";
    if (vcatrec.find(pref) != vcatrec.end())
    {
        auto it = vappcat.find(matecckey);
        if (it != vappcat.end())
        {
            vappcat[matecckey].clear();
            vappcat[matecckey].insert(pref);
        }
    }
    if (CATDBG) //print all
    {
        std::map<string, cset> vcatapp;
        for (auto ac : vappcat)
            for (auto ca : ac.second)
                vcatapp[ca].insert(ac.first);

        for (auto e : vcatapp)
        {
            printf("\n=== Category %s", e.first.c_str());
            for (auto key : e.second)
            {
                printf("\n  %s", vappmap[key]->fname.c_str());
            }
        }
    }
}

//#define printoper(lev,desc,s1,s2,res) dbg("\n%s%s %s %s = %s",lev.c_str(),desc,set2buf(s1).c_str(),set2buf(s2).c_str(),set2buf(res).c_str())
void CategoryLoader::setIntersect(cset& s1, const cset& s2)
{
    for (cset::iterator it = s1.begin(); it != s1.end();)
    {
        if (s2.find(*it) == s2.end())
            s1.erase(it++);
        else
            ++it;
    }
}

void CategoryLoader::setSubtract(cset& s1, const cset& s2)
{
    for (cset::iterator it = s1.begin(); it != s1.end();)
    {
        if (s2.find(*it) != s2.end())
            s1.erase(it++);
        else
            ++it;
    }
}

void CategoryLoader::setUnion(cset& s1, const cset& s2)
{
    s1.insert(s2.begin(), s2.end());
}

cset CategoryLoader::xmlProcessInclude(xmlNodePtr xmlNode, const cset& setin, int plevel)
{
    static const auto AND = "And";
    static const auto OR = "Or";
    static const auto NOT = "Not";
    cset xset;
    if (xmlNode == NULL)
        return xset;
    string xtag = xnodeName(xmlNode);
    dbg("\nxmlProcessInclude tag <%s>", xtag.c_str());
    string slevel = sutl::format("%*s", 6 * plevel, "");
    string xtagval = xtag == "Category" ? xnodeText(xmlNode) : "";
    if (xtag == "Include")
    {
        auto xlist = children(xmlNode);
        cset result;
        for (size_t i = 0; i < xlist.size(); i++)
        {
            auto xchild = xlist[i];
            cset r = xmlProcessInclude(xchild, setin, plevel + 1);
            setUnion(result, r);
        }
        dbg("\n%sOUT result2 %s", slevel.c_str(), set2buf(result).c_str());
        return result;
    }

    else if (xtag == AND || xtag == OR || xtag == NOT)
    {
        auto xlist = children(xmlNode);
        bool xinit = false;
        cset xsetchild;
        for (size_t i = 0; i < xlist.size(); i++)
        {
            auto xchild = xlist[i];
            string xchildtag = xnodeName(xchild);

            string xtagval = xchildtag == "Category" ? xnodeText(xchild) : "";

            dbg("\n%si=%i child tag <%s> %s", slevel.c_str(), (int) i, xchildtag.c_str(), xtagval.c_str());
            if (xchildtag == "")
                continue;
            xsetchild = xmlProcessInclude(xchild, setin, plevel + 1);
            dbg("\n%schild set %s", slevel.c_str(), set2buf(xsetchild).c_str());
            if (!xinit)
            {
                xset = xsetchild;
                xinit = true;
            } else
            {
                if (xtag == AND)
                    setIntersect(xset, xsetchild);
                if (xtag == OR)
                    setUnion(xset, xsetchild);
                if (xtag == NOT)
                    setUnion(xset, xsetchild); //Union!

            }
            dbg("\n%sxset %s", slevel.c_str(), set2buf(xset).c_str());
            if (xtag == AND && xset.empty())
                break;

        }

        if (xinit && xtag == NOT)
        {
            cset s = setin;
            setSubtract(s, xset);
            xset = s;
        }

    } else if (xtag == "All")
    {
        xset = setin;
    } else if (xtag == "Filename")
    {

        string fname = xnodeText(xmlNode);
        dbg("%sFilename %s", slevel.c_str(), fname.c_str());
        xset.clear();
        for (auto e : vappmap)
        {
            string cmpname = e.second->fname;
            if (!sutl::contains(fname, "/"))
                cmpname = Toolkit().shortName(cmpname);
            if (fname == cmpname)
                xset.insert(e.first);
        }
    } else if (xtag == "Category")
    {
        string catname = xnodeText(xmlNode);
        dbg("%sCategory %s", slevel.c_str(), catname.c_str());
        xset.clear();
        for (auto e : vappmap)
        {
            if (e.second->vcatset.find(catname) != e.second->vcatset.end())
                xset.insert(e.first);
        }
    }

    dbg("\n%sOUT result %s", slevel.c_str(), set2buf(xset).c_str());

    return xset;
}

void CategoryLoader::loadCategoryDirFiles()
{
    if (vdirectories.empty()) //initialize cache once
    {
        Toolkit toolkit;
        vector<string> cdirs;
        vector<string> vdir;
        cdirs.push_back("/usr/share/mate/desktop-directories/");
        cdirs.push_back("/var/lib/menu-xdg/desktop-directories/menu-xdg/");
        cdirs.push_back("/usr/share/desktop-directories/");

        for (auto dir : cdirs)
        {
            if (toolkit.dirExists(dir))
                toolkit.getDirectory(dir, &vdir);
        }

        for (auto file : vdir)
        {
            if (!sutl::endsWith(file, ".directory"))
                continue;
            string shortName = toolkit.shortName(file);
            vdirectories[shortName].push_back(file);
        }
    }
}

void CategoryLoader::getDirectoryData(CategoryRec& crec)
{
    string fname = crec.dirFile;
    assert(!fname.empty());
    Toolkit t;
    ConfigMap tmap;
    string shortName = t.shortName(fname);
    auto it = vdirectories.find(shortName);
    if (it == vdirectories.end())
    {
        return;
    }
    string path;
    if (sutl::contains(fname, "/"))
    {
        for (auto f : it->second)
            if (f == fname)
            {
                path = f;
                break;
            }
    }

    if (path.empty())
        path = it->second[0];
    ApplicationLoader::loadMap(path, tmap);
    crec.loctitle = ApplicationLoader::getMapLocValue(tmap, "name");
    crec.comment = tmap["comment"];
    crec.loccomment = ApplicationLoader::getMapLocValue(tmap, "comment");
    crec.iconName = tmap["icon"];
    crec.dirFilePath = path;

}

void CategoryLoader::free()
{
    for (auto mf : vmenufiles)
    {
        if (mf.xmlDoc != NULL)
        {
            xmlFreeDoc(mf.xmlDoc);
        }
    }
    xmlCleanupParser();
    vmenus = std::vector<MenuFilePos>();
    vmenufiles = std::vector<MenuFile>();
    vdirectories = std::map<string, std::vector<string>>();

    vappcat = std::map<string, cset>();
    vappmap = std::map<string, DesktopRec*>();
    vcatrec = std::map<string, CategoryRec>();
}

