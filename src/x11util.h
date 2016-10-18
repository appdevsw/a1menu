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

class X11KeyData
{
public:

    enum KeyboardModifier
    {
        NoModifier = 0x00000000,
        ShiftModifier = 0x02000000,
        ControlModifier = 0x04000000,
        AltModifier = 0x08000000,
        MetaModifier = 0x10000000,
        KeypadModifier = 0x20000000,
    };

    std::set<int> codes;
    string toString();

    void createFromString(const string& seq);
private:
    static std::map<int, string> vKeyNames;
    static std::map<int, string> vModNames;
    static std::map<int, int> vModBits;


    static void init();
    static std::string toKeyName(int xkeycode);
    static int toKeyCode(const string& keyname);
    static bool isModifier(int xkeycode);
    static X11KeyData getKeyDataFromCodes(std::set<int> codes);
    static bool isAnyKeyPressed();
};

class X11KeyListener
{
public:

    typedef void (*callbackfun)(const X11KeyData& kd);
    typedef char kbdstate[32];
    X11KeyListener();
    virtual ~X11KeyListener();
    void setKey(const string& keyseq);
    void setCallback(callbackfun);
    static X11KeyData getKeyboardState(Display *display = NULL);
    void suspend(bool par=true);
private:
    callbackfun callback = NULL;
    std::atomic<int> started { 0 };
    std::atomic<int> doStop { 0 };
    X11KeyData currentKey;
    std::thread thr;
    static std::map<Display *, int> vErrors;
    Select socketWaiter;
    volatile bool suspended=false;


    bool checkXI2();
    void run();
    static void runner(X11KeyListener * kl);
    static int errorHandler(Display* display, XErrorEvent *event);

    int start();
    void stop();

};

#endif /* QT_LNK_MENU_X11UTIL_H_ */
