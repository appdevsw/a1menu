#ifndef A1MENU_GTK_SRC_DESKTOPREC_H_
#define A1MENU_GTK_SRC_DESKTOPREC_H_

#include "configmap.h"
#include <set>

//do not change names , they are used inside quotes "..."

struct DesktopRec
{
    std::string fname;
    std::string iconName;
    std::string command;
    std::string title;
    std::string loctitle;
    std::string comment;
    std::string loccomment;
    std::string genname;
    std::string locgenname;
    std::string categories;
    std::vector<std::string> vcategories;
    std::set<std::string> vcatset;
    std::string category;
    std::string itemtype;
    ConfigMap attr;
    std::string attrs;
    int key;
};

#endif /* A1MENU_GTK_SRC_DESKTOPREC_H_ */
