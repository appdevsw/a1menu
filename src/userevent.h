#ifndef TIMEREXEC_H
#define TIMEREXEC_H

#include <QObject>
#include <QEvent>
#include <atomic>

class Item;
class QPixmap;
class ToolButton;
class Loader;

namespace ctx
{
extern const QString cfgData;
}

enum UserEventType
{
	FIT_LABELS = QEvent::User + 100, //
	APPLET_SHOW = QEvent::User + 101, //
	CONFIG_DIALOG = QEvent::User + 102, //
	SYNCHRONIZE_MENU = QEvent::User + 103, //
	SET_PANEL_BUTTON = QEvent::User + 104, //
	IPC_RELOAD = QEvent::User + 105, //
	BTN_RELOAD = QEvent::User + 106, //
	LOADER = QEvent::User + 107, //
};

class UserEvent: public QObject, public QEvent
{
Q_OBJECT
public:

	UserEvent(UserEventType type);
	virtual ~UserEvent();
	void postLater(int delay);

	struct custom_data
	{
		Item * item = NULL;
		int serial = 0;
		struct
		{
			QString path=NULL;
			QPixmap * pix=NULL;
			ToolButton * tbut=NULL;
			Item * item=NULL;
			int size;
		} icon;
		bool clearCache=false;
	} custom={};

public slots:
	void onTimerData();

};
#endif // TIMEREXEC_H
