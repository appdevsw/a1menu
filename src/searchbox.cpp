#include <searchbox.h>
#include <ctx.h>
#include <userevent.h>
#include <QPainter>
#include <QMouseEvent>

SearchBox::SearchBox(QWidget * parent) :
		QLineEdit(parent)
{
}

void SearchBox::paintEvent(QPaintEvent * event)
{
	QLineEdit::paintEvent((QPaintEvent*) event);
	QPainter painter(this);
	int y = height() / 2 - isize / 2;
	int x = width() - isize - space1;
	if (text() != "")
		painter.drawPixmap(QPoint(x, y), pixclear);
	else
		painter.drawPixmap(QPoint(x, y), pixsearch);
}

void SearchBox::setIcons(QString isearch, QString iclear,int isize)
{
	this->isize = isize;
	pixsearch = QIcon(isearch).pixmap(isize, isize);
	pixclear = QIcon(iclear).pixmap(isize, isize);
	auto m = textMargins();
	m.setRight(isize + space1 + space2);
	setTextMargins(m);

}

void SearchBox::mousePressEvent(QMouseEvent * event)
{
	if (text() != "" && event->pos().x() >= (width() - isize - space1))
		setText("");
}

void SearchBox::mouseMoveEvent(QMouseEvent * event)
{
	if (text() != "" && event->pos().x() >= (width() - isize - space1))
		this->setCursor(Qt::ArrowCursor);
	else
		this->setCursor(Qt::IBeamCursor);
	QLineEdit::mouseMoveEvent(event);

}

bool SearchBox::event(QEvent * event)
{
	QString txt = this->text();
	if (prevText != txt)
		prevText = txt;
	if (callback != NULL)
		callback->onSearchBoxCallback(this,event,txt);
	return QWidget::event(event);
}

void SearchBox::setCallback(SearchBoxCallback * callback)
{
	this->callback = callback;
}

SearchBoxCallback::SearchBoxCallback()
{
}
SearchBoxCallback::~SearchBoxCallback()
{
}
