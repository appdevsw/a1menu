#ifndef A1MENU_GTK_SRC_INOTIFYMONITOR_H_
#define A1MENU_GTK_SRC_INOTIFYMONITOR_H_

#include <string>
#include <thread>
#include <atomic>
#include <map>
#include <set>
#include "select.h"

class inotify_event;

class INotifyMonitor
{
public:
    INotifyMonitor();
    virtual ~INotifyMonitor();

    void addPath(std::string path);
    void stop();
    bool check();

private:
    void mainLoop();
    void displayInotifyEvent(inotify_event *i);
    volatile int inotifyFd = -1;
    volatile bool doStop=false;
    unsigned MODIFY_MASK = 0;
    std::thread * thr=NULL;
    std::atomic<int> changeCounter { 0 };
    std::map<int,std::string> vWatches;
    std::set<std::string> vWatchesNonExistent;
    Select socketWaiter;


};

#endif /* A1MENU_GTK_SRC_INOTIFYMONITOR_H_ */
