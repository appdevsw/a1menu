#ifndef QT_LNK_MENU_X11UTIL_H_
#define QT_LNK_MENU_X11UTIL_H_
#include <QLineEdit>
#include <QEvent>
#include <map>
#include <atomic>

class X11Util
{
public:
	X11Util();
	virtual ~X11Util();
	void setForegroundWindow(int wid);
};


class KeyListenerThread;

class X11KeyListener
{
public:

	typedef char kbdstate[32];
	X11KeyListener();
	virtual ~X11KeyListener();
	int start();
	void stop();
	void lockListener(int enable);
	void setHotKeyFromString(QString keyseq);
	QString getKeyboardState(kbdstate& keys);
	void run();

private:
	void setKey(int nativeModifiers, int nativeKeyCode);
	std::atomic<int> started { 0 };
	std::atomic<int> doStop { 0 };
	int lock = 0;
	int modifiers = 0;
	int keyCode = 0;
	QString keyseq;
	KeyListenerThread * klthread=NULL;

};


class HotKeyField: public QWidget
{
public:
	struct keydata
	{
		int code = 0;
		int modifiers = 0;
		QString text; //
		bool valid = false;
	};

	HotKeyField();
	~HotKeyField(); //
	bool eventFilter(QObject *obj, QEvent *event);
	void setText(QString & txt);
	QString text();

	static QString keyEventToString(QKeyEvent * event);
	static keydata parseKeySeq(QString keyseq);

private:

	int handleHotKeyEvent(QEvent * e);
	QString displayedText();
	QString orgText;
	QLineEdit * le = NULL;
	static std::map<int, QString> vkmodifiers;
	static std::map<int, QString> vmodcodes;

};

#endif /* QT_LNK_MENU_X11UTIL_H_ */
