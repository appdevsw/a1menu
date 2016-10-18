#include "guicombobox.h"
using std::string;

GuiComboBox::GuiComboBox()
{
    combo = gtk_combo_box_text_new();
    g_object_ref(combo);

}

GuiComboBox::~GuiComboBox()
{
    g_object_unref(combo);
}

GtkWidget * GuiComboBox::widget()
{
    return combo;
}

void GuiComboBox::add(string key, string label)
{
    gtk_combo_box_text_append_text((GtkComboBoxText*) combo, label.c_str());
    vkeys.push_back(key);
}

string GuiComboBox::get()
{
    int i = gtk_combo_box_get_active((GtkComboBox*) combo);
    if (i >= 0)
        return vkeys[i];
    return "";
}

void GuiComboBox::set(string key)
{
    int i = 0;
    for (auto k : vkeys)
    {
        if (k == key)
        {
            gtk_combo_box_set_active((GtkComboBox*) combo, i);
            break;
        }
        i++;
    }
}
