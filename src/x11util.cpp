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

Display * X11KeySequence::kdisplay = NULL;
set<int> X11KeySequence::vModifiers;

void X11KeySequence::init()
{
    if (kdisplay == NULL)
        kdisplay = XOpenDisplay(NULL); //opened forever

    if (vModifiers.empty())
    {
        vModifiers.insert(XK_Caps_Lock);
        vModifiers.insert(XK_Shift_L);
        vModifiers.insert(XK_Shift_R);
        vModifiers.insert(XK_Control_L);
        vModifiers.insert(XK_Control_R);
        vModifiers.insert(XK_Alt_L);
        vModifiers.insert(XK_Alt_R);
        vModifiers.insert(XK_Super_L);
        vModifiers.insert(XK_Super_R);
    }
}

string X11KeySequence::toKeyName(int xkeycode)
{
    init();
    auto keysym = XkbKeycodeToKeysym(kdisplay, xkeycode, 0, 0);
    if (keysym == NoSymbol)
        return sutl::format("Unknown_Key_%i", xkeycode);
    return XKeysymToString(keysym);
}

int X11KeySequence::toKeyCode(const string& keyname)
{
    init();
    auto keysym = XStringToKeysym(keyname.c_str());
    if (keysym == NoSymbol)
        return -1;
    auto xkeycode = XKeysymToKeycode(kdisplay, keysym);
    if (xkeycode == 0)
        return -1;
    return xkeycode;
}

bool X11KeySequence::isModifier(int xkeycode)
{
    init();
    auto keysym = XkbKeycodeToKeysym(kdisplay, xkeycode, 0, 0);
    return vModifiers.find(keysym) != vModifiers.end();
}

void X11KeySequence::createFromString(const string& seq)
{
    codes.clear();
    vector<string> v;
    sutl::tokenize(seq, v, " ");
    for (auto keyname : v)
    {
        codes.insert(toKeyCode(keyname));
    }
}

string X11KeySequence::toString()
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

X11KeyListener::X11KeyListener()
{
}

X11KeyListener::~X11KeyListener()
{
    stop();
}

void X11KeyListener::setCallback(callbackfun callback)
{
    mutexChangeKey.lock();
    this->callback = callback;
    mutexChangeKey.unlock();
}

int X11KeyListener::start()
{
    assert(!currentKey.toString().empty());
    if (started)
        return -1;
    started = 0;
    thr = thread(&X11KeyListener::run, this);
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
    X11KeySequence kd;
    kd.createFromString(keyseq);
    mutexChangeKey.lock();
    currentKey = kd;
    mutexChangeKey.unlock();
    if (keyseq.empty())
    {
        stop();
        return;
    }
    suspended = false;
    start();
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

    if (!checkXI2())
    {
        printf("\nHotKey listener error: XI2 is not available");
        started = 0;
        return;
    }

    printf("\nHotKey listener started with key: <%s>", currentKey.toString().c_str());

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
        XISetMask(m->mask, XI_KeyRelease);
        XISetMask(m->mask, XI_RawKeyRelease);
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

    set<int> vStateDelta;
    while (!doStop)
    {

        if (!XPending(display))
        {
            socketWaiter.add(fdx);
            auto r = socketWaiter.wait(10000);
            if (r < 0)
                break;
        }

        if (doStop)
            break;

        while (XPending(display))
        {

            XEvent ev;
            XGenericEventCookie *cookie = &ev.xcookie;
            XNextEvent(display, (XEvent*) &ev);
            if (!XGetEventData(display, cookie))
                continue;
            XIDeviceEvent *evd = (XIDeviceEvent *) cookie->data;

            //printf("\nev %i", evd->evtype);

            if (evd->evtype == XI_RawKeyRelease || evd->evtype == XI_KeyRelease)
                vStateDelta.erase(evd->detail);
            else if (evd->evtype == XI_RawKeyPress || evd->evtype == XI_KeyPress)
            {
                vStateDelta.insert(evd->detail);

                if (suspended)
                    continue;

                mutexChangeKey.lock();
                auto cmp = callback != NULL && currentKey.codes == vStateDelta;
                mutexChangeKey.unlock();

                if (cmp)
                {
                    printf("\nHotKey <%s> pressed.", currentKey.toString().c_str());
                    callback(currentKey);
                }
            }
        }
    }
    XCloseDisplay(display);
    started = 0;
    printf("\nHotKey listener stopped.");

}

X11KeySequence X11KeyListener::getKeyboardState(Display * pdisplay)
{
    X11KeySequence kd;
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
