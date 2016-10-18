#ifndef A1MENU_GTK_SRC_SELECT_H_
#define A1MENU_GTK_SRC_SELECT_H_

#include <set>
#include <mutex>
#include <unistd.h>
#include <sys/select.h>

class Select
{
public:
    Select();
    virtual ~Select();
    void add(int fd);
    void interrupt();
    bool isSet(int fd);
    int wait(int timeoutMs);
private:
    int pipefd[2] { 0 };
    std::set<int> vfds;
    fd_set fdsetin;
    struct timeval tv;
    std::mutex minit;
    void ensurePipe();

};

#endif /* A1MENU_GTK_SRC_SELECT_H_ */
