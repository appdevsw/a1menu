#include "loader.h"
#include <map>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <gtk/gtk.h>
#include <sys/stat.h>
#include "applicationloader.h"
#include "toolkit.h"
#include "configmap.h"
#include "config.h"
#include "sutl.h"
#include "application.h"
#include "properties.h"

using namespace std;

Loader::Loader()
{
}

Loader::~Loader()
{
}

void Loader::clearCache()
{
    if (!Toolkit().dirExists(app->dirCache))
        return;
    printf("\nloader: clear cache");
    string cmd;
    cmd = "find " + app->dirCache + " -type f -name '*.png' -delete";
    system(cmd.c_str());
    //cmd = "find " + app->dircache + " -type f -name '*.txt' -delete";
    //system(cmd.c_str());
    unlink(app->loadFile.c_str());
}

string Loader::iconKey(const string& iconName, int iconSize)
{
    return iconName + ":" + sutl::itoa(iconSize);
}

string Loader::toCachedPath(const string& iconPathOrg, int iconSize)
{
    string iconPath = sutl::replace(iconPathOrg, "/", "!");
    return sutl::format("%s./%s:%i.png", app->dirCacheIcons.c_str(), iconPath.c_str(), iconSize);
}

string Loader::getCachedIconPath(const string& iconName, int iconSize)
{
    string key = iconKey(iconName, iconSize);
    auto it = vicons.find(key);
    if (it == vicons.end())
    {
        printf("\nIcon %s:%i not found in cache!", iconName.c_str(), iconSize);
        return "";
    }
    return it->second;
}

GtkWidget * Loader::getCachedIcon(const string& iconName, int iconSize)
{
    string path = getCachedIconPath(iconName, iconSize);
    if (path.empty())
        return NULL;
    return gtk_image_new_from_file(path.c_str());
}

GdkPixbuf * Loader::getCachedPixbuf(const string& iconName, int iconSize)
{
    string path = getCachedIconPath(iconName, iconSize);
    if (path.empty())
        return NULL;
    return gdk_pixbuf_new_from_file(path.c_str(), NULL);
}

string Loader::createCachedIcon(const string& iconName, int iconSize)
{

    GdkPixbuf *pix = NULL, *pixbuiltin = NULL;
    GtkIconInfo * info = NULL;
    Toolkit t;
    string iconPath;
    string iconPathCached;

    try
    {

        if (iconTheme == NULL)
        {

            iconTheme = gtk_icon_theme_get_default();
            assert(iconTheme != NULL);
        }
        if (sutl::contains(iconName, "/"))
            iconPath = iconName;

        if (iconPath.empty())
        {
            GtkIconLookupFlags flags = (GtkIconLookupFlags) GTK_ICON_LOOKUP_USE_BUILTIN;
            string iconName2 = iconName;
            if (sutl::contains(iconName2, "."))
            {
                iconName2 = sutl::replace(iconName2, ".png", "");
                iconName2 = sutl::replace(iconName2, ".svg", "");
                iconName2 = sutl::replace(iconName2, ".xpm", "");
            }
            info = gtk_icon_theme_lookup_icon(iconTheme, iconName2.c_str(), iconSize, flags);
            if (info != NULL)
            {
                pixbuiltin = gtk_icon_info_get_builtin_pixbuf(info);
                if (pixbuiltin == NULL)
                {
                    iconPath = gtk_icon_info_get_filename(info);
                }

                if (sutl::startsWith(iconPath, "/org/") && pixbuiltin == NULL)
                    pixbuiltin = gdk_pixbuf_new_from_resource(iconPath.c_str(), NULL);
            }
        }
        //printf("\ncache icon %s %s", iconName.c_str(), ipath.c_str());

        if (iconPath.empty())
        {
            if (pixbuiltin == NULL)
            {
                printf("\n *** cache icon %s not found", iconName.c_str());
                throw -1;
            }
            iconPath = "builtin:" + iconName;
        }
        iconPathCached = toCachedPath(iconPath, iconSize);

        struct stat st { 0 };
        stat(iconPathCached.c_str(), &st);

        if (st.st_size > 0)
            return iconPathCached;

        pix = pixbuiltin;
        if (pix == NULL)
        {
            GError * error = NULL;
            pix = gdk_pixbuf_new_from_file_at_scale(iconPath.c_str(), iconSize, iconSize, true, &error);
            if (pix == NULL)
            {
                printf("\ngdk_pixbuf_new_from_file_at_scale error %s %s", iconName.c_str(), (error ? error->message : ""));
                if (error)
                    g_error_free(error);
                throw -2;
            }
        }
        assert(pix!=NULL);
        GError * error = NULL;
        auto r = gdk_pixbuf_save(pix, iconPathCached.c_str(), "png", &error, "compression", "0", NULL);
        if (pixbuiltin == NULL) //do not unref stock
            g_object_unref(pix);
        if (error != NULL)
        {
            printf("\nIcon caching error %i %s\n", r, error->message);
            g_error_free(error);
            throw -3;
        }
    } catch (int e)
    {
        iconPathCached = "";
    }

    if (info)
        gtk_icon_info_free(info);
    return iconPathCached;
}

void Loader::loadProc()
{
    Toolkit toolkit;
    app->config->load();

    string cmd;

#define createdir(d) cmd = string("mkdir -p ") + app-> d;system(cmd.c_str());assert(toolkit.dirExists(app-> d));

    createdir(dirCache);
    createdir(dirCacheIcons);
    createdir(dirProperties);
    createdir(dirTranslation);

    std::ofstream fstr;
    fstr.open(app->loadFile, std::ios::out | std::ios::binary);
    assert(fstr.is_open());

    struct IconRec
    {
        string name;
        int size;
    };
    vector<IconRec> vicons;

    ApplicationLoader appLoader;
    appLoader.getItems(vapplications);
    CategoryLoader catLoader;
    catLoader.init(&vapplications);

    appLoader.insertPlaces(vapplications);

    //------------- load applications

    for (auto deskrec : vapplications)
    {

        //printf("\napp %s %s", deskrec.title.c_str(),deskrec.category.c_str());

        ConfigMap cmap;
        cmap["itemtype"] = res.itemApp;

        deskrec.attrs = sutl::encode(deskrec.attr.pack(), attrEscape);

#define rec2map(name) cmap[#name]=deskrec.name
        rec2map(iconName);
        rec2map(title);
        rec2map(genname);
        rec2map(loctitle);
        rec2map(locgenname);
        rec2map(command);
        rec2map(comment);
        rec2map(loccomment);
        rec2map(fname);
        rec2map(categories);
        rec2map(attrs);
#undef rec2map

        string catMoveTo = app->properties->get(deskrec, res.pkeyMoveToCategory);

        int iconSize = CFGI("app_icon_size");

        CategoryLoader::CategoryRec catrec;
        if (!catMoveTo.empty())
        {
            catrec.category = catMoveTo;
        } else if (sutl::startsWith(deskrec.fname, toolkit.desktopDir()))
        {
            catrec.category = res.categoryDesktop;
            catrec.loctitle = res.categoryDesktop_loc;
            catrec.iconName = CFG("icon_place_desktop");
        } else if (deskrec.category == res.categoryPlaces)
        {
            catrec.category = res.categoryPlaces;
            catrec.iconName = CFG("icon_category_places");
            cmap["itemtype"] = res.itemPlace;
            iconSize = CFGI("place_icon_size");
        } else
        {
            catrec = catLoader.getCategory(deskrec);
            if (catrec.category.empty())
            {
                //catrec.category = res.categoryOther;
                continue;
            }
        }
        vicons.push_back( { deskrec.iconName, iconSize });

        if (vcategories.find(catrec.category) == vcategories.end())
        {
            vcategories[catrec.category] = catrec;
        }

        cmap["category"] = catrec.category;

        string raw = cmap.pack();
        raw = sutl::encode(raw);
        fstr << raw << "\n";

    }

    //------------- load categories

    for (auto ec : vcategories)
    {
        auto & crec = ec.second;

        ConfigMap cmap;
        cmap["itemtype"] = res.itemCategory;

#define rec2map(name) cmap[#name]=crec.name;
        rec2map(category);
        rec2map(dirFile);
        rec2map(dirFilePath);
        rec2map(loctitle);
        rec2map(comment);
        rec2map(loccomment);
        rec2map(iconName);
#undef rec2map

        vicons.push_back( { crec.iconName, CFGI("category_icon_size") });

        string raw = cmap.pack();
        raw = sutl::encode(raw);
        fstr << raw << "\n";

    }

    //------------- load icons
    int iconSize = CFGI("category_icon_size");
    vicons.push_back( { CFG("icon_category_all"), iconSize });
    vicons.push_back( { CFG("icon_category_other"), iconSize });
    vicons.push_back( { CFG("icon_category_places"), iconSize });
    vicons.push_back( { CFG("icon_category_recent"), iconSize });
    vicons.push_back( { CFG("icon_category_favorites"), iconSize });

    iconSize = CFGI("toolbar_icon_size");
    vicons.push_back( { CFG("icon_toolbar_shutdown"), iconSize });
    vicons.push_back( { CFG("icon_toolbar_settings"), iconSize });
    vicons.push_back( { CFG("icon_toolbar_logout"), iconSize });
    vicons.push_back( { CFG("icon_toolbar_lockscreen"), iconSize });
    vicons.push_back( { CFG("icon_toolbar_reload"), iconSize });

    vicons.push_back( { CFG("icon_menu"), app->menuIconSize });

    vicons.push_back( { res.iconSearchBoxName, res.iconSearchBoxSize });

    vicons.push_back( { CFG("icon_category_places"), CFGI("place_icon_size") });

    for (auto iconrec : vicons)
    {
        if (iconrec.name.empty())
            continue;
        string cachePath = createCachedIcon(iconrec.name, iconrec.size);
        ConfigMap cmap;
        cmap["itemtype"] = res.itemIcon;
        cmap["name"] = iconrec.name;
        cmap["size"] = sutl::itoa(iconrec.size);
        cmap["path"] = cachePath;
        string raw = cmap.pack();
        raw = sutl::encode(raw);
        fstr << raw << "\n";
    }

    fstr.close();
    catLoader.free();
}

void Loader::clientLoad()
{
    ifstream istr;
    istr.open(app->loadFile);
    assert(!istr.fail());
    string line;
    while (std::getline(istr, line))
    {
        ConfigMap cmap;
        cmap.clear();
        string raw = sutl::decode(line);
        cmap.unpack(raw);

        if (cmap["itemtype"] == res.itemApp || cmap["itemtype"] == res.itemPlace)
        {
            DesktopRec deskrec;

#define rec2map(name) deskrec.name=cmap[#name];
            rec2map(iconName);
            rec2map(title);
            rec2map(genname);
            rec2map(loctitle);
            rec2map(locgenname);
            rec2map(command);
            rec2map(comment);
            rec2map(loccomment);
            rec2map(fname);
            rec2map(categories);
            rec2map(category);
            rec2map(itemtype);
            rec2map(attrs);
#undef rec2map

            string dec = sutl::decode(deskrec.attrs, attrEscape);
            deskrec.attr.unpack(dec);
            vapplications.push_back(deskrec);
        }

        if (cmap["itemtype"] == res.itemCategory)
        {
            CategoryLoader::CategoryRec catrec;

#define rec2map(name) catrec.name=cmap[#name];
            rec2map(category);
            rec2map(dirFile);
            rec2map(dirFilePath);
            rec2map(loctitle);
            rec2map(comment);
            rec2map(loccomment);
            rec2map(iconName);
#undef rec2map

            vcategories[catrec.category] = catrec;
        }

        if (cmap["itemtype"] == res.itemIcon)
        {
            string key = iconKey(cmap["name"], atoi(cmap["size"].c_str()));
            auto path = cmap["path"];
            if (!path.empty())
                vicons[key] = path;
        }

    }
    istr.close();
}

void Loader::runInProcess(int argc, char ** argv, bool clearCache)
{
    bool inSeparateProcess = true;
    Toolkit t;
    auto t1 = t.GetMicrosecondsTime();
    printf("\nloader started.");
    if (clearCache)
        this->clearCache();

    if (inSeparateProcess)
    {

        string cmd = string(argv[0]) + " --load";
        int r = system(cmd.c_str());
        if (r)
        {
            printf("\nLoader error: return code: %i, exit status: %i", r, WEXITSTATUS(r));
            exit(r);
        }
    } else
    {
        printf("\nloader is running in the same process");
        auto loader = Loader();
        loader.loadProc();
    }
    clientLoad();
    auto t2 = t.GetMicrosecondsTime();
    printf("\nloader finished (%lld ms)", (t2 - t1) / 1000);
}
