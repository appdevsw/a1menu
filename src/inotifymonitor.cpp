#include "inotifymonitor.h"
#include "toolkit.h"
#include <sys/inotify.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <assert.h>

//This code is based on examples from linux manuals (http://man7.org/tlpi/code/online/book/inotify/demo_inotify.c.html)

using namespace std;

INotifyMonitor::INotifyMonitor()
{
    MODIFY_MASK = IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVE_SELF | IN_IGNORED;
}

INotifyMonitor::~INotifyMonitor()
{
    stop();
}

void INotifyMonitor::addPath(string path)
{
    doStop = false;
    if (inotifyFd <= 0)
    {
        //inotifyFd = inotify_init1(IN_NONBLOCK); //not working well
        inotifyFd = inotify_init();
        assert(thr == NULL);
        thr = new thread(&INotifyMonitor::mainLoop, this);
    }

    if (!Toolkit().dirExists(path))
    {
        vWatchesNonExistent.insert(path);
        return;
    }

    int fd = inotify_add_watch(inotifyFd, path.c_str(), MODIFY_MASK); //IN_ALL_EVENTS);
    if (fd == -1)
    {
        printf("\ninotify error %i %s", fd, path.c_str());
        return;
    }
    vWatches[fd] = path;
}

void INotifyMonitor::stop()
{
    doStop = true;
    if (inotifyFd > 0)
    {
        printf("\nClosing INotifyMonitor...");
        vWatchesNonExistent.clear();
        addPath("/tmp/");
        doStop = true; //after add
        for (auto e : vWatches)
        {
            inotify_rm_watch(inotifyFd, e.first); //and getenrate IN_IGNORED event, that kicks the waiting socket
        }

    }
    if (thr != NULL)
    {
        thr->join();
        delete thr;
        thr = NULL;
        printf("\nINotifyMonitor closed.");
    }

    if (inotifyFd > 0)
    {
        close(inotifyFd);
        inotifyFd = -1;
    }
}

void INotifyMonitor::displayInotifyEvent(struct inotify_event *i)
{
    printf("\n    wd =%2d; ", i->wd);
    if (i->cookie > 0)
        printf("cookie =%4d; ", i->cookie);

    printf("mask = ");
    if (i->mask & IN_ACCESS)
        printf("IN_ACCESS ");
    if (i->mask & IN_ATTRIB)
        printf("IN_ATTRIB ");
    if (i->mask & IN_CLOSE_NOWRITE)
        printf("IN_CLOSE_NOWRITE ");
    if (i->mask & IN_CLOSE_WRITE)
        printf("IN_CLOSE_WRITE ");
    if (i->mask & IN_CREATE)
        printf("IN_CREATE ");
    if (i->mask & IN_DELETE)
        printf("IN_DELETE ");
    if (i->mask & IN_DELETE_SELF)
        printf("IN_DELETE_SELF ");
    if (i->mask & IN_IGNORED)
        printf("IN_IGNORED ");
    if (i->mask & IN_ISDIR)
        printf("IN_ISDIR ");
    if (i->mask & IN_MODIFY)
        printf("IN_MODIFY ");
    if (i->mask & IN_MOVE_SELF)
        printf("IN_MOVE_SELF ");
    if (i->mask & IN_MOVED_FROM)
        printf("IN_MOVED_FROM ");
    if (i->mask & IN_MOVED_TO)
        printf("IN_MOVED_TO ");
    if (i->mask & IN_OPEN)
        printf("IN_OPEN ");
    if (i->mask & IN_Q_OVERFLOW)
        printf("IN_Q_OVERFLOW ");
    if (i->mask & IN_UNMOUNT)
        printf("IN_UNMOUNT ");
    printf("\n");

    if (i->len > 0)
        printf("        name = %s\n", i->name);
}

void INotifyMonitor::mainLoop()
{
    int BUF_LEN(10 * (sizeof(struct inotify_event) + NAME_MAX + 1));
    char buf[BUF_LEN] __attribute__ ((aligned(8)));
    Toolkit t;
    int timeout = 5000;
    for (;;)
    {

        if (inotifyFd < 0 || doStop)
            break;

        for (auto path : vWatchesNonExistent)
            if (t.dirExists(path))
            {
                addPath(path);
                vWatchesNonExistent.erase(path);
                changeCounter++;
                printf("\nINotifyMonitor: add new path %s", path.c_str());
            }

        socketWaiter.add(inotifyFd);
        int n = socketWaiter.wait(timeout);
        if (n <= 0 || !socketWaiter.isSet(inotifyFd))
            continue;

        int numRead = read(inotifyFd, buf, BUF_LEN);
        if (numRead <= 0)
        {
            printf("\nINotifyMonitor error: read() returned %i !", numRead);
            break;
        }

        //printf("\nRead %ld bytes from inotify fd\n", (long) numRead);

        char *p;
        for (p = buf; p < buf + numRead;)
        {
            auto event = (struct inotify_event *) p;
            //displayInotifyEvent(event);
            if (event->len > 0)
                printf("\nINotifyMonitor: file changed: %s", event->name);
            changeCounter++;
            p += sizeof(struct inotify_event) + event->len;
        }
    }

    if (inotifyFd > 0)
    {
        close(inotifyFd);
        inotifyFd = -1;
    }

}

bool INotifyMonitor::check()
{
    int sum = 0;
    while (changeCounter != 0)
    {
        int c = changeCounter;
        sum += c;
        changeCounter -= c;
    }
    return sum != 0;
}
