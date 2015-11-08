#include <userevent.h>
#include <QTimer>
#include <QEvent>
#include <QApplication>
#include "ctx.h"
#include "mainwindow.h"
#include "toolkit.h"
#include <vector>
#include <set>

using std::vector;
using std::set;

UserEvent::UserEvent(UserEventType type) :
		QEvent((QEvent::Type)type)
{
}

UserEvent::~UserEvent()
{
	//qDebug("UserEvent destructor %i",(int)type());

}

void UserEvent::onTimerData()
{
	//qDebug("UserEvent::onTimerData() send %i", (int) this->type());
	QApplication::postEvent(ctx::wnd, this);
}

void UserEvent::postLater(int delay)
{
	if (delay <= 0)
		QApplication::postEvent(ctx::wnd, this);
	else
		QTimer::singleShot(delay, (QObject*) this, SLOT(onTimerData()));
}

