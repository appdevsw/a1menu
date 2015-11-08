#ifndef TOOLBUTTON_H
#define TOOLBUTTON_H

#include <QWidget>
#include <QPushButton>
#include <QEvent>
#include "userevent.h"

class ToolButton: public QPushButton
{
public:
	ToolButton(QWidget *parent = 0);
//   virtual void keyPressEvent(QKeyEvent * event);
	virtual bool event(QEvent * event);
	void setToolTip(const QString &);
	void mouseDoubleClickEvent(QMouseEvent * event);
	void setUserEvent(UserEventType type);
	static bool isButtonPressedEvent(QEvent * event);
	QString tooltip;
	QString command;
	//bool styleSet = false;
private:
	UserEventType userEventType = (UserEventType)0;

};

#endif // TOOLBUTTON_H
