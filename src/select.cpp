#include "select.h"
#include <assert.h>
#include <stdio.h>
#include <fcntl.h>
#include <string>
#include <string.h>

Select::Select()
{
}

Select::~Select()
{
    minit.lock();
    if (pipefd[0] > 0)
    {
        close(pipefd[0]);
        close(pipefd[1]);
    }
    minit.unlock();
}

void Select::ensurePipe()
{
    minit.lock();
    if (pipefd[0] == 0)
    {
        assert(pipe2(pipefd, O_NONBLOCK) != -1);
    }
    minit.unlock();
}

void Select::add(int fd)
{
    vfds.insert(fd);
}

void Select::interrupt()
{
    ensurePipe();
    write(pipefd[1], "X", 1);
}

bool Select::isSet(int fd)
{
    return FD_ISSET(fd, &fdsetin);
}

int Select::wait(int timeoutMs)
{
    ensurePipe();
    vfds.insert(pipefd[0]);
    FD_ZERO(&fdsetin);
    int fdmax = -1;
    for (auto fd : vfds)
    {
        FD_SET(fd, &fdsetin);
        if (fdmax < fd)
            fdmax = fd;
    }
    if (timeoutMs >= 0)
    {
        tv.tv_usec = (timeoutMs % 1000) * 1000; //microsec
        tv.tv_sec = timeoutMs / 1000;
    }
    vfds.clear();
    int r = select(fdmax + 1, &fdsetin, 0, 0, timeoutMs ? &tv : NULL);
    if (FD_ISSET(pipefd[0], &fdsetin))
    {
        char c;
        while (read(pipefd[0], &c, 1) > 0)
            ;
        if (r > 0)
            r--;
    }
    return r;
}
