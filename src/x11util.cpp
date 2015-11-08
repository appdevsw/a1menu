#include "ctx.h"
#include <unistd.h>
#include <assert.h>
#include <x11util.h>
#include <mainwindow.h>
#include <QEvent>
#include <QKeyEvent>
#include <QHBoxLayout>
#include <QThread>
#include <map>

using std::map;

map<int, QString> HotKeyField::vkmodifiers;
map<int, QString> HotKeyField::vmodcodes;

HotKeyField::HotKeyField() :
		QWidget()
{
	if (vkmodifiers.empty())
	{
		//Qt code
		vkmodifiers[Qt::NoModifier] = " ";
		vkmodifiers[Qt::ShiftModifier] = "Shift";
		vkmodifiers[Qt::ControlModifier] = "Control";
		vkmodifiers[Qt::AltModifier] = "Alt";
		vkmodifiers[Qt::MetaModifier] = "Meta";
		vkmodifiers[Qt::KeypadModifier] = "Keypad";

		//native x11 scan code
		vmodcodes[0x32] = "Shift_L";
		vmodcodes[0x25] = "Control_L";
		vmodcodes[0x85] = "Meta_L";
		vmodcodes[0x40] = "Alt_L";
		vmodcodes[0x6C] = "Alt_R";
		vmodcodes[0x86] = "Meta_R";
		vmodcodes[0x69] = "Control_R";
		vmodcodes[0x3E] = "Shift_R";
	} //if
	ctx::application->installEventFilter(this);
	le = new QLineEdit();
	QHBoxLayout *layout = new QHBoxLayout(this);
	layout->setMargin(0);
	layout->addWidget(le);

}
HotKeyField::~HotKeyField()
{
	ctx::application->removeEventFilter(this);
	delete le;
}

void HotKeyField::setText(QString & txt)
{
	orgText = txt;
	le->setText(displayedText());
}
QString HotKeyField::text()
{
	return orgText;
}

QString HotKeyField::displayedText()
{

	return parseKeySeq(orgText).text;
}

bool HotKeyField::eventFilter(QObject *obj, QEvent *event)
{
	if (obj == le)
	{
		if (le->text() != displayedText())
			le->setText(displayedText());
		if (obj == le && handleHotKeyEvent(event))
			return true;
	}
	return false;
}

QString HotKeyField::keyEventToString(QKeyEvent * ke)
{
	QString sm;
	for (auto m : vkmodifiers)
		if ((m.first & ke->modifiers()) || m.first == ke->modifiers())
			sm += m.second + "+";
	if (sm == "")
		return "";
	sm.replace(" +", "");
	int keycode = ke->key();
	int kn = ke->nativeScanCode();
	QString keytxt = QS(QKeySequence(keycode).toString());
	if (keytxt.contains("?"))
	{
		if (vmodcodes.find(kn) != vmodcodes.end())
			keytxt = vmodcodes[kn];
		else
			keytxt.sprintf("[0x%X]", keycode);
	}
	QString res = sm + "<" + keytxt + ">  :" + QString::number(ke->nativeModifiers()) + ":" + QString::number(ke->nativeScanCode());
	return res;
}

HotKeyField::keydata HotKeyField::parseKeySeq(QString keyseq)
{
	keydata d;
	auto spl = keyseq.split(":");
	if (spl.size() == 3)
	{
		d.text = spl[0];
		d.modifiers = spl[1].toInt();
		d.code = spl[2].toInt();
		d.valid = true;
	}
	return d;
}

int HotKeyField::handleHotKeyEvent(QEvent * event)
{
	int tp = event->type();
	QKeyEvent *ke = dynamic_cast<QKeyEvent *>(event);
	if (ke == NULL)
		return 0;
	QString keyseq = keyEventToString(ke);
	if ((tp == QEvent::KeyPress || ke->key() == Qt::Key_Escape))
	{
		if (keyseq.startsWith("<Backspace>"))
			keyseq = " ";
		if (keyseq != "" && keyseq != text())
		{
			setText(keyseq);
		}
	}
	return 1;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

#include <QX11Info>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>

X11KeyListener::X11KeyListener()
{
}

X11KeyListener::~X11KeyListener()
{
	lock=0;
	stop();
}

class KeyListenerThread: public QThread
{
public:
	X11KeyListener * listener;
	void run()
	{
		listener->run();
	}
};

int X11KeyListener::start()
{
	if (lock)
		return -2;
	assert(keyCode != 0);
	if (started)
		return -1;
	started = 0;
	assert(klthread==NULL);

	klthread = new KeyListenerThread();
	klthread->listener = this;

	klthread->start();
	while (!started)
		usleep(1000);
	return 0;
}

void X11KeyListener::stop()
{
	if (lock)
		return;
	if (!started)
		return;
	assert(klthread != NULL);
	doStop = 1;
	klthread->wait();
	doStop = 0;
	delete klthread;
	klthread=NULL;
}

void X11KeyListener::lockListener(int enable)
{
	lock = enable;
}

void X11KeyListener::setKey(int nativeModifiers, int nativeKeyCode)
{

	this->modifiers = nativeModifiers;
	this->keyCode = nativeKeyCode;
}

void X11KeyListener::setHotKeyFromString(QString keyseq)
{
	if (this->keyseq == keyseq && started)
		return;
	auto d = HotKeyField::parseKeySeq(keyseq);
	stop();
	if (d.valid)
	{
		setKey(d.modifiers, d.code);
		this->keyseq = keyseq;
		start();
	}
}

void X11KeyListener::run()
{
	X11KeyListener * obj = this;
	obj->started = 1;
	printf("HotKey listener started with key: %s\n", QS(obj->keyseq));
	Display *display = XOpenDisplay(NULL);
	Window win = QX11Info::appRootWindow();
	XGrabKey(display, obj->keyCode, obj->modifiers, win, true, GrabModeAsync, GrabModeAsync);
	while (!obj->doStop)
	{
		XEvent e;
		while (XPending(display))
		{
			//printf("\nx11 event %i\n", e.type);
			XNextEvent(display, &e);
			if (e.type == KeyPress)
			{
				printf("HotKey event: key code: 0x%02X , key state: 0x%08X\n", e.xkey.keycode, e.xkey.state);
				ctx::wnd->sendShowEvent();
				while (XPending(display))
					XNextEvent(display, &e);
			}
		}
		usleep(50 * 1000);
	}
	XUngrabKey(display, obj->keyCode, obj->modifiers, win);
	XCloseDisplay(display);
	//printf("Exit x11 loop\n");
	obj->started = 0;
	printf("HotKey listener stopped.\n");
}

QString X11KeyListener::getKeyboardState(kbdstate& keys)
{
	Display *display = XOpenDisplay(NULL);
	XQueryKeymap(display, keys);
	XCloseDisplay(display);
	/*
	 for (int i = 0; i < (int) sizeof(keys); i++)
	 printf("%02X ", (keys[i] & 0XFF));
	 printf("\n");
	 */
	/*
	 for (int i = 0; i < (int) sizeof(keys); i++)
	 if(keys[i])
	 printf("%2i: %02X ", i, (keys[i] & 0XFF));
	 printf("\n");
	 */
	QString code;
	for (int i = 0; i < (int) sizeof(keys); i++)
		if (keys[i])
		{
			code += QString().sprintf("%2i: %02X;", i, (keys[i] & 0XFF));
		}
	return code;

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
