#include "x11util.h"
#include "sutl.h"
#include <sstream>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <X11/XKBlib.h>

#include <gdk/gdkx.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/XInput.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

using namespace std;

std::map<int, string> X11KeyData::vKeyNames;
std::map<int, string> X11KeyData::vModNames;
std::map<int, int> X11KeyData::vModBits;

void X11KeyData::init()
{
    if (!vKeyNames.empty())
        return;
    //XSetErrorHandler(errorHandler);

    Display *display = XOpenDisplay(NULL);

    //modifiers

    /*
     Mask        | Value | Key
     ------------+-------+------------
     ShiftMask   |     1 | Shift
     LockMask    |     2 | Caps Lock
     ControlMask |     4 | Ctrl
     Mod1Mask    |     8 | Alt
     Mod2Mask    |    16 | Num Lock
     Mod3Mask    |    32 | Scroll Lock
     Mod4Mask    |    64 | Windows
     Mod5Mask    |   128 | ???
     */

#define insmod(keysym) vModNames[XKeysymToKeycode(display,keysym)]=XKeysymToString(keysym)
    insmod(XK_Caps_Lock);
    insmod(XK_Shift_L);
    insmod(XK_Shift_R);
    insmod(XK_Control_L);
    insmod(XK_Control_R);
    insmod(XK_Alt_L);
    insmod(XK_Alt_R);
    insmod(XK_Super_L);
    insmod(XK_Super_R);

#define insmodm(keysym,bit) vModBits[XKeysymToKeycode(display,keysym)]=bit

    insmodm(XK_Caps_Lock, 2);
    insmodm(XK_Shift_L, 1);
    insmodm(XK_Shift_R, 1);
    insmodm(XK_Control_L, 4);
    insmodm(XK_Control_R, 4);
    insmodm(XK_Alt_L, 8);
    insmodm(XK_Alt_R, 8);
    insmodm(XK_Super_L, 64);
    insmodm(XK_Super_R, 64);

    /* find masks programatically, but what about right modifiers?
     auto mm = XGetModifierMapping(display);
     for (int i = 0; i < 8; i++)
     {
     auto kcode=mm->modifiermap[mm->max_keypermod * i];
     auto mask= 1 << i;
     printf("\nmod %i %i %i ", i, kcode,mask);
     }*/

    for (int xkeycode = 0; xkeycode < 256; xkeycode++)
    {
        auto keysym = XkbKeycodeToKeysym(display, xkeycode, 0, 0);
        if (keysym != NoSymbol)
        {
            char * kname = XKeysymToString(keysym);
            if (vModNames.find(xkeycode) == vModNames.end())
            {
                if (kname != NULL && kname[0])
                {

                    //printf("\ninit %i %lu %s", xkeycode, keysym, kname);
                    vKeyNames[xkeycode] = string(kname);
                }
            } else
            {
                //printf("\ninit %i %lu %s !modifier", xkeycode, keysym, kname);

            }
        }

    }

    XCloseDisplay(display);
}

string X11KeyData::toKeyName(int xkeycode)
{
    init();
    auto it = vKeyNames.find(xkeycode);
    if (it != vKeyNames.end())
        return it->second;

    it = vModNames.find(xkeycode);
    if (it != vModNames.end())
        return it->second;

    return "key" + sutl::itoa(xkeycode);
}

int X11KeyData::toKeyCode(const string& keyname)
{
    init();
    auto arr = { &vKeyNames, &vModNames };
    for (auto m : arr)
        for (auto e : *m)
        {
            if (e.second == keyname)
                return e.first;
        }
    return -1;
}

bool X11KeyData::isModifier(int xkeycode)
{
    init();
    return vModNames.find(xkeycode) != vModNames.end();
}

void X11KeyData::createFromString(const string& seq)
{
    codes.clear();
    vector<string> v;
    sutl::tokenize(seq, v, " ");
    for (auto keyname : v)
    {
        codes.insert(toKeyCode(keyname));
    }
}

string X11KeyData::toString()
{
    string strkeys;
    string strmods;
    for (auto xkeycode : codes)
    {
        string keyname = toKeyName(xkeycode);
        if (isModifier(xkeycode))
        {
            strmods += keyname + " ";
        } else
        {
            strkeys += keyname + " ";
        }
    }
    return sutl::trim(strmods + strkeys);
}

//////////////////////////////////////////////////////////////////////////////////////////////

std::map<Display *, int> X11KeyListener::vErrors;

X11KeyListener::X11KeyListener()
{
}

X11KeyListener::~X11KeyListener()
{
    stop();
}

void X11KeyListener::setCallback(callbackfun callback)
{
    this->callback = callback;
}

void X11KeyListener::runner(X11KeyListener * kl)
{
    kl->run();
}

int X11KeyListener::start()
{
    assert(!currentKey.toString().empty());
    if (started)
        return -1;
    started = 0;
    thr = thread(runner, this);
    while (!started)
        usleep(1000);
    return 0;
}

void X11KeyListener::stop()
{
    if (!started)
        return;
    doStop = 1;
    socketWaiter.interrupt();
    thr.join();
    doStop = 0;
}

void X11KeyListener::suspend(bool par)
{
    suspended = par;
}

void X11KeyListener::setKey(const string& keyseq)
{
    printf("\nset hotkey to <%s>", keyseq.c_str());
    X11KeyData kd;
    kd.createFromString(keyseq);
    currentKey = kd;
    if (keyseq.empty())
    {
        stop();
        return;
    }
    suspended = false;
    start();
}

int X11KeyListener::errorHandler(Display* display, XErrorEvent *event)
{
    vErrors[display] = 1;
    printf("\nX11KeyListener error");
    return 1;
}

bool X11KeyListener::checkXI2()
{
    Display * display = XOpenDisplay(NULL);
    int opcode, event, error, major = 2, minor = 1;
    bool result = true;
    if (!XQueryExtension(display, "XInputExtension", &opcode, &event, &error))
    {
        result = false;
    }
    if (result)
        if (XIQueryVersion(display, &major, &minor) == BadRequest)
        {
            result = false;
        }

    XCloseDisplay(display);
    return result;
}

void X11KeyListener::run()
{
    started = 1;
    printf("\nHotKey listener started with key: <%s>", currentKey.toString().c_str());
    //XSetErrorHandler(errorHandler);

    if (!checkXI2())
    {
        printf("\nHotKey listener error: XI2 not available");
        started = 0;
        return;
    }

    {
        Display * display = XOpenDisplay(NULL);
        int devcount;
        XDeviceInfo *devices = XListInputDevices(display, &devcount);
        int imaskmax = 64;
        XIEventMask mask[imaskmax] { 0 };
        Window win = DefaultRootWindow(display);

        int idx = 0;
        for (int i = 0; i < devcount; i++)
        {
            assert(idx < imaskmax);
            string devname = sutl::lower(devices[i].name);
            if (!sutl::contains(devname, "keyboard"))
                continue;
            //printf("\nSelect input device: %3lu %s", devices[i].id, devices[i].name);
            XIEventMask *m;
            m = &mask[idx];
            m->deviceid = devices[i].id;
            m->mask_len = XIMaskLen(XI_LASTEVENT);
            m->mask = (unsigned char *) calloc(m->mask_len, 1); //0-memory
            XISetMask(m->mask, XI_KeyPress);
            XISetMask(m->mask, XI_RawKeyPress);
            idx++;
        }
        assert(idx > 0);

        XISelectEvents(display, win, &mask[0], 2);
        XSync(display, False);

        for (int i = 0; i < idx; i++)
        {
            free(mask[i].mask);
        }

        auto fdx = ConnectionNumber(display);

        while (!doStop)
        {

            if (!XPending(display))
            {
                socketWaiter.add(fdx);
                auto r = socketWaiter.wait(10000);
                //printf("\nselect %i", r);
                if (r < 0)
                    break;
            }

            if (!XPending(display))
                continue;

            XEvent ev;
            //printf("\nevent pre");
            XNextEvent(display, (XEvent*) &ev);

            if (doStop)
                break;
            //printf("\nevent %i", ev.type);

            if (suspended)
                continue;

            auto currState = getKeyboardState(display);
            if (callback != NULL && currentKey.codes == currState.codes)
            {
                printf("\nHotKey <%s> pressed.", currState.toString().c_str());
                callback(currState);
            }

        }
        XCloseDisplay(display);
        started = 0;
        printf("\nHotKey listener stopped.");
    }
}

X11KeyData X11KeyListener::getKeyboardState(Display * pdisplay)
{
    X11KeyData kd;
    kbdstate keys;
    Display *display = pdisplay == NULL ? XOpenDisplay(NULL) : pdisplay;
    XQueryKeymap(display, keys);
    for (unsigned i = 0; i < sizeof(keys); i++)
    {
        auto cc = keys[i];
        for (int j = 0; j < 8; j++)
        {
            bool isKeyPressed = ((1 << j) & cc) != 0;
            if (isKeyPressed)
            {
                KeyCode xkeycode = i * 8 + j;
                kd.codes.insert(xkeycode);
            }
        }
    }

    if (pdisplay == NULL)
        XCloseDisplay(display);
    return kd;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

X11Util::X11Util()
{
}

X11Util::~X11Util()
{
}

void X11Util::setForegroundWindow(int wid)
{
    Display *display = XOpenDisplay(NULL);
    XEvent event = { 0 };
    event.xclient.type = ClientMessage;
    event.xclient.serial = 0;
    event.xclient.send_event = True;
    event.xclient.message_type = XInternAtom(display, "_NET_ACTIVE_WINDOW", False);
    event.xclient.window = wid;
    event.xclient.format = 32;
    XSendEvent(display, DefaultRootWindow(display), False, SubstructureRedirectMask | SubstructureNotifyMask, &event);
    XMapRaised(display, wid);
    XCloseDisplay(display);
}

void X11Util::setPosition(int wid, int x, int y)
{
    Display *display = XOpenDisplay(NULL);
    /*
     XSizeHints size_hints;
     size_hints.flags = USPosition;
     size_hints.x = x;
     size_hints.y = y;
     XSetNormalHints(display, wid, &size_hints);
     */
    XMoveWindow(display, wid, x, y);
    XCloseDisplay(display);
}
