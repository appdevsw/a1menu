#include "properties.h"
#include "sutl.h"
#include "guiitem.h"
#include "application.h"
#include "toolkit.h"

using std::string;

Properties::Properties()
{
}

Properties::~Properties()
{
    for (auto e : cache)
        delete e.second;
}

string Properties::getHash(DesktopRec& rec)
{
    string keyBase = sutl::replace(rec.fname, "/", "~");
    if (keyBase.empty())
        keyBase = rec.title + rec.itemtype;
    string md5 = Toolkit().md5(keyBase);
    return md5;
}

string Properties::filePath(string hash)
{
    return string(app->dirProperties + hash);
}

ConfigMap * Properties::getMap(DesktopRec& rec)
{
    string hash = getHash(rec);
    ConfigMap * cmap = cache[hash];
    if (cmap == NULL)
    {
        cmap = cache[hash] = new ConfigMap();
        cmap->load(filePath(hash));
    }
    return cmap;
}

string Properties::get(DesktopRec& rec, const string& key)
{

    ConfigMap * cmap = getMap(rec);
    return (*cmap)[key];
}

bool Properties::getbool(DesktopRec& rec, const string& key)
{
    return !get(rec, key).empty();
}

void Properties::set(DesktopRec& rec, const string& key, const string& val)
{
    ConfigMap * cmap = getMap(rec);
    (*cmap)[key] = val;
    bool anyExists = false;
    for (auto e : cmap->getMap())
    {
        anyExists = !e.second.empty();
        if (anyExists)
            break;
    }
    string hash = getHash(rec);
    if (anyExists)
        cmap->save(filePath(hash));
    else
        unlink(filePath(hash).c_str());
}

void Properties::addRow(const string& labtxt, GtkWidget * field, bool visible)
{

    auto label = gtk_label_new((labtxt + ":").c_str());
    guint count;
    gtk_table_get_size((GtkTable*) table, &count, NULL);
    GtkAttachOptions optx = (GtkAttachOptions) (GTK_EXPAND | GTK_FILL);
    gtk_misc_set_alignment(GTK_MISC(label), 1, .5);
    gtk_table_attach((GtkTable*) table, label, 0, 1, count, count + 1, optx, GTK_FILL, 2, 2);
    gtk_table_attach((GtkTable*) table, field, 1, 2, count, count + 1, optx, GTK_FILL, 2, 2);
    gtk_widget_set_visible(label, visible);
    gtk_widget_set_visible(field, visible);
}

void Properties::openForm(DesktopRec& rec)
{
    string title = res.labProperties + " [" + rec.title + "]";
    auto pwnd = gtk_dialog_new_with_buttons(title.c_str(),
    NULL, (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
    GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
    GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
    NULL);

    gtk_window_set_transient_for((GtkWindow*) pwnd, (GtkWindow*) app->currentWindow()->widget());

    auto content = gtk_dialog_get_content_area(GTK_DIALOG(pwnd));
    table = gtk_table_new(0, 0, 0);

    gtk_container_add(GTK_CONTAINER(content), table);
    gtk_widget_show_all(content);

    bool isApp = rec.itemtype == res.itemApp || rec.itemtype == res.itemPlace;

    auto aliase = gtk_entry_new();
    addRow(res.labCatAlias.c_str(), aliase, !isApp);

    auto defcate = gtk_check_button_new();
    addRow(res.labDefaultCategory.c_str(), defcate, !isApp);

    auto infe = gtk_check_button_new();
    addRow(res.labInFavorites.c_str(), infe, isApp);

    auto movecate = gtk_entry_new();
    addRow(res.labMoveToCategory.c_str(), movecate, isApp);

    gtk_entry_set_text((GtkEntry*) aliase, get(rec, res.pkeyCategoryAlias).c_str());
    gtk_entry_set_text((GtkEntry*) movecate, get(rec, res.pkeyMoveToCategory).c_str());
    gtk_toggle_button_set_active((GtkToggleButton*) defcate, getbool(rec, res.pkeyDefaultCategory));
    gtk_toggle_button_set_active((GtkToggleButton*) infe, getbool(rec, res.pkeyInFavorites));

    gtk_window_set_position((GtkWindow*) pwnd, GTK_WIN_POS_MOUSE);
    gtk_widget_show(pwnd);

    int r;
    for (;;)
    {
        r = gtk_dialog_run((GtkDialog*) pwnd);
        if (r == GTK_RESPONSE_REJECT)
        {
            break;
        }

        if (r == GTK_RESPONSE_ACCEPT)
        {
            set(rec, res.pkeyCategoryAlias, gtk_entry_get_text((GtkEntry*) aliase));
            set(rec, res.pkeyMoveToCategory, gtk_entry_get_text((GtkEntry*) movecate));
            auto infav = gtk_toggle_button_get_active((GtkToggleButton*) infe);
            set(rec, res.pkeyInFavorites, infav ? res.parYes : "");
            auto defcat = gtk_toggle_button_get_active((GtkToggleButton*) defcate);
            set(rec, res.pkeyDefaultCategory, defcat ? res.parYes : "");
            if (defcat)
            {
                for (auto i : *(app->currentWindow()->catList->items()))
                {
                    if (i->rec.title != rec.title)
                        set(i->rec, res.pkeyDefaultCategory, "");
                }
            }
            app->reload(false);
            break;
        }
    }
    gtk_widget_destroy(pwnd);
    app->setBlocked(false);
}
