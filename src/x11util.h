#ifndef QT_LNK_MENU_X11UTIL_H_
#define QT_LNK_MENU_X11UTIL_H_

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>

#include <map>
#include <set>
#include <atomic>
#include <string>
#include <thread>
#include <mutex>
#include "select.h"

using std::string;

class X11Util
{
public:
    X11Util();
    virtual ~X11Util();
    static void setForegroundWindow(int wid);
    static void setPosition(int wid, int x, int y);
};

class X11KeySequence
{
public:

    std::set<int> codes;
    string toString();

    void createFromString(const string& seq);
private:
    static std::set<int> vModifiers;
    static Display * kdisplay;

    static void init();
    static std::string toKeyName(int xkeycode);
    static int toKeyCode(const string& keyname);
    static bool isModifier(int xkeycode);
    static X11KeySequence getKeyDataFromCodes(std::set<int> codes);
    static bool isAnyKeyPressed();
};

class X11KeyListener
{
public:

    typedef void (*callbackfun)(const X11KeySequence& kd);
    X11KeyListener();
    virtual ~X11KeyListener();
    void setKey(const string& keyseq);
    void setCallback(callbackfun);
    static X11KeySequence getKeyboardState(Display *display = NULL);
    void suspend(bool par=true);
private:
    typedef char kbdstate[32];
    callbackfun callback = NULL;
    std::atomic<int> started { 0 };
    std::atomic<int> doStop { 0 };
    X11KeySequence currentKey;
    std::mutex mutexChangeKey;
    std::thread thr;
    Select socketWaiter;
    volatile bool suspended=false;


    bool checkXI2();
    void run();
    int start();
    void stop();
};

#endif /* QT_LNK_MENU_X11UTIL_H_ */
