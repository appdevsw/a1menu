#include "guiparametersform.h"
#include "sutl.h"
#include "application.h"
#include "application.h"
#include "x11util.h"
#include "toolkit.h"

#include <assert.h>
#include <algorithm>
#include <thread>

using namespace std;

GuiParametersForm::GuiParametersForm()
{
}

GuiParametersForm::~GuiParametersForm()
{
    for (auto it : vitems)
        delete it;
}

GtkWidget * GuiParametersForm::widget()
{
    return pwnd;
}

int GuiParametersForm::create(int parentPID)
{
    assert(parentPID > 1);
    this->parentPID = parentPID;
    config.load();

    pwnd = gtk_dialog_new_with_buttons(_("Menu preferences"),
    NULL, (GtkDialogFlags) (GTK_DIALOG_MODAL),
            GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
            GTK_STOCK_APPLY, GTK_RESPONSE_APPLY,
            GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
            NULL);

    auto content = gtk_dialog_get_content_area(GTK_DIALOG(pwnd));
    auto scroll = gtk_scrolled_window_new( NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start((GtkBox*) content, scroll, 1, 1, 0);

    table = gtk_table_new(0, 0, 0);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scroll), table);

    for (auto cfgit : *config.items())
    {
        cfgit->value = config.mapref()[cfgit->name];
        if (cfgit->hidden)
            continue;
        PFormItem * it = new PFormItem(this);
        it->configItem = cfgit;

        for (auto i : vitems) //parent has a lower index and it is already there
            if (i->configItem == cfgit->parent)
            {
                it->parent = i;
                break;
            }
        it->createGui();
        vitems.push_back(it);

    }

    buildGuiTree(0);

    gtk_window_resize((GtkWindow*) pwnd, 800, 600);
    gtk_window_set_position(GTK_WINDOW(pwnd), GTK_WIN_POS_CENTER_ALWAYS);
    gtk_widget_show_all(content);
    gtk_widget_show_all(widget());

    Toolkit t;
    string h1 = t.md5(config.mapref().pack());

    int r;
    for (;;)
    {
        r = gtk_dialog_run((GtkDialog*) pwnd);
        //printf("\ndialog result %i\n", r);
        if (r == GTK_RESPONSE_REJECT)
        {
            break;
        }

        int vres = validateForm();
        if (vres == -1)
            continue;
        config.save();
        string h2 = t.md5(config.mapref().pack());

        if (r == GTK_RESPONSE_ACCEPT)
        {
            if (h1 != h2)
            {
                kill(parentPID, app->SIG_RELOAD);
            }
            break;
        }
        if (r == GTK_RESPONSE_APPLY)
        {
            kill(parentPID, app->SIG_RELOAD);
        }
        h1 = h2;
    }
    gtk_widget_destroy(pwnd);
    return r;
}

void GuiParametersForm::buildGuiTree(ConfigItem * parent, int level)
{
    assert(level < 99 && "Error in the tree structure");
    map<string, PFormItem *> vsort;

    for (auto it : vitems)
    {
        if (it->configItem->parent == parent)
        {
            string sort = sutl::format("%1i %5i", (it->configItem->isCategory ? 1 : 0), it->configItem->ord);
            sort += it->configItem->label + " " + sutl::itoa(it->configItem->id);
            vsort[sort] = it;
        }
    }

    for (auto kv : vsort)
    {
        PFormItem * it = vsort[kv.first];
        if (it->level() == 1 && it->configItem->isCategory)
            it->collapsed = true;

        guint count;
        gtk_table_get_size((GtkTable*) table, &count, NULL);
        GtkAttachOptions optx = (GtkAttachOptions) (GTK_EXPAND | GTK_FILL);
        gtk_table_attach((GtkTable*) table, it->labelBox, 0, 1, count, count + 1, GTK_FILL, GTK_SHRINK, 0, 0);
        if (it->field != NULL)
            gtk_table_attach((GtkTable*) table, it->fieldbox, 1, 2, count, count + 1, optx, GTK_SHRINK, 0, 0);

        buildGuiTree(it->configItem, level + 1);
    }
}

#define raiseError(msg) error=msg,throw 1

int GuiParametersForm::validateForm()
{
    string error = "";
    string value;
    for (int apply = 0; apply <= 1; apply++)
        for (auto it : vitems)
        {
            auto& cit = it->configItem;
            try
            {
                if (cit->isCategory)
                    continue;
                if (it->button != NULL)
                    continue;

                if (it->guicombo != NULL)
                {
                    value = it->guicombo->get();
                } else if (it->hotkeygtk != NULL)
                    value = gtk_entry_get_text((GtkEntry*) it->hotkeygtk);
                else
                    value = gtk_entry_get_text((GtkEntry*) it->entry);

                if (apply)
                {
                    cit->value = value;
                    config.mapref()[cit->name] = value;
                    continue;
                }

                if (value == "" && cit->mandatory)
                    raiseError(_("Value must be entered"));

                if (cit->type == res.parIntType)
                {
                    if (!value.empty())
                    {
                        int vali = atoi(value.c_str());
                        if (value != sutl::itoa(vali))
                            raiseError(_("Invalid integer"));
                        else
                        {

                            if ((cit->rangeMin != "" && vali < atoi(cit->rangeMin.c_str()))
                                    || (cit->rangeMax != "" && vali > atoi(cit->rangeMax.c_str())))
                            {
                                raiseError(string(_("Range error")) + " (" + cit->rangeMin + ":" + cit->rangeMax + ")");
                            }

                        }
                    }
                }
                if (cit->validItems.size() > 0)
                {
                    auto v = std::find(cit->validItems.begin(), cit->validItems.end(), value);
                    if (v == cit->validItems.end())
                        raiseError(_("Value is not allowed"));
                }
            } catch (int e)
            {
                if (error != "")
                {
                    string bmsg = sutl::format("\n%s\n\nparameter: `%s`\nvalue: `%s`", error.c_str(), cit->labelorg.c_str(), value.c_str());
                    auto dialog = gtk_message_dialog_new((GtkWindow*) pwnd, //
                            GTK_DIALOG_DESTROY_WITH_PARENT, //
                            GTK_MESSAGE_ERROR, //
                            GTK_BUTTONS_CLOSE, bmsg.c_str());
                    gtk_window_set_title((GtkWindow*) dialog, "Validation error");
                    gtk_dialog_run(GTK_DIALOG(dialog));
                    gtk_widget_destroy(dialog);
                    return -1;
                }
            }
        }

    return 0;
}

#undef raiseError

volatile bool GuiParametersForm::isThreadRunning;

void GuiParametersForm::threadProc()
{
    string cmd = string(app->argv[0]) + " --pform " + sutl::itoa(getpid());
    system(cmd.c_str());
    isThreadRunning = false;
}

int GuiParametersForm::run()
{

    app->keyListener->suspend(true);
    isThreadRunning = true;
    app->setBlocked(true);
    thread thr(threadProc);

    while (isThreadRunning)
    {
        while (gtk_events_pending())
            gtk_main_iteration_do(true);
        usleep(10 * 1000);
    }
    app->setBlocked(false);
    thr.join();
    app->config->load();
    app->keyListener->setKey(CFG("hotkey"));
    return 0;

}

/*
 *
 *
 *
 ---------------  PFormItem  ----------------
 *
 *
 *
 */

PFormItem::PFormItem(GuiParametersForm * pform)
{
    this->pform = pform;
}

PFormItem::~PFormItem()
{
    if (guicombo != NULL)
        delete guicombo;
}

int PFormItem::level()
{
    int l = 0;
    auto p = parent;
    while (p != NULL)
    {
        l++;
        p = p->parent;
    }
    return l;
}

int PFormItem::isChildOf(int id)
{
    PFormItem * p = parent;
    int l = 0;
    while (p != NULL)
    {
        l++;
        if (p->configItem->id == id)
            return l;
        p = p->parent;
    }
    return 0;
}

gboolean PFormItem::onButtonClicked(GtkWidget *widget, void *data)
{
    PFormItem * it = (PFormItem *) data;
    if (it->configItem->name == "button_clear_cache")
    {
        kill(it->pform->parentPID, app->SIG_CLEAR_CACHE);
    }
    return TRUE;
}

void PFormItem::createGui()
{
    auto labLevelPadding = gtk_label_new("");
    gtk_widget_set_size_request(labLevelPadding, level() * 20, -1);
    labelBox = gtk_hbox_new(false, 0);

    auto label = gtk_label_new(configItem->labelorg.c_str());
    gtk_misc_set_alignment(GTK_MISC(label), 0, .5); //range (0.0 - 1.0) left <-> right

    gtk_box_pack_start((GtkBox*) labelBox, labLevelPadding, 0, 0, 0);
    gtk_box_pack_start((GtkBox*) labelBox, label, 0, 0, 0);

    if (level() == 0)
        gtk_label_set_text((GtkLabel*) label, "");
    else if (configItem->isCategory)
    {
        gtk_widget_set_size_request(labelBox, -1, 50);

        string fsize = "medium";
        if (level() == 0)
            fsize = "x-large";
        if (level() == 1)
            fsize = "large";
        string bold = sutl::format("<span font_size='%s' font_weight='bold'  >%s</span>", fsize.c_str(), configItem->labelorg.c_str());
        gtk_label_set_markup(GTK_LABEL(label), bold.c_str());
    }

    if (!configItem->isCategory)
    {
        fieldbox = gtk_hbox_new(false, 0);
        if (configItem->hotkey)
        {
            hotkeyobj = new GuiHotKeyField();
            hotkeyobj->create();
            field = hotkeygtk = hotkeyobj->widget();
            gtk_entry_set_text((GtkEntry*) field, configItem->value.c_str());
        } else if (configItem->validItems.size() > 0)
        {
            guicombo = new GuiComboBox();
            field = guicombo->widget();
            for (auto key : configItem->validItems)
            {
                string label = app->mapTranslations[key];
                if (label.empty())
                    label = key;
                guicombo->add(key, label);
            }
            guicombo->set(configItem->value);

        } else if (configItem->buttonId > 0)
        {
            button = field = gtk_button_new_with_label(configItem->label.c_str());
            gtk_label_set_text((GtkLabel*) label, "");
            g_signal_connect(G_OBJECT (button), "clicked", G_CALLBACK (onButtonClicked), this);

        } else
        {
            entry = field = gtk_entry_new();
            gtk_entry_set_text((GtkEntry*) field, configItem->value.c_str());
        }

        auto expand = hotkeygtk != NULL || (entry != NULL && configItem->type == 0);
        gtk_box_pack_start((GtkBox*) fieldbox, field, expand, expand, 0);
    }
}

