#include "toolbutton.h"
#include "toolkit.h"
#include "ctx.h"
#include "userevent.h"
#include "mainwindow.h"
#include <QMouseEvent>
#include <QCoreApplication>

using namespace ctx;

ToolButton::ToolButton(QWidget *parent) :
		QPushButton(parent)
{
}

bool ToolButton::isButtonPressedEvent(QEvent * event)
{
	QMouseEvent * me = dynamic_cast<QMouseEvent *>(event);
	QKeyEvent * ke = dynamic_cast<QKeyEvent *>(event);
	return (me != NULL && me->type() == QEvent::MouseButtonPress) || (ke != NULL && ke->type() == QEvent::KeyPress);
}

void ToolButton::mouseDoubleClickEvent(QMouseEvent * event)
{
	if (command == "quit")
		ctx::wnd->quitApp();
}

void ToolButton::setUserEvent(UserEventType type)
{
	userEventType=type;
}

bool ToolButton::event(QEvent * event)
{
	bool run = isButtonPressedEvent(event);
	if ((command != "" && command!="quit") && run)
	{
		qDebug("run %s", QS(command));
		Toolkit toolkit;
		toolkit.runCommand(command);
		ctx::wnd->hide();
		return true;
	}

	if (userEventType && run)
	{
		//qDebug("ToolButton post event %i",userEventType);
		UserEvent * e = new UserEvent(userEventType);
		QCoreApplication::postEvent(this, e);
		return true;
	}

	return QPushButton::event(event);
}

void ToolButton::setToolTip(const QString & txt)
{
	QPushButton::setToolTip(txt);
}

