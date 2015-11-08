#include "itemlabel.h"
#include "ctx.h"
#include <QPainter>

ItemLabel::ItemLabel() :
		QLabel()
{

}

ItemLabel::~ItemLabel()
{
}

QString ItemLabel::text()
{
	return QLabel::text();
}

void ItemLabel::setText(const QString & txt)
{
	this->orgText = txt;
	QLabel::setText(txt);
}

void ItemLabel::paintEvent(QPaintEvent * event)
{
	QLabel::paintEvent((QPaintEvent*) event);
	int w = event->rect().width();
	if (prevPaintWidth == w)
		return;
	prevPaintWidth = w;
	if (trimMode)
		trimToSize(w);
}

void ItemLabel::enableTrimMode(bool enable)
{
	trimMode = enable;
	setWordWrap(!enable);
}

int ItemLabel::getWidth()
{
	return this->fontMetrics().boundingRect(this->text()).width();
}

void ItemLabel::trimToSize(int maxWidth)
{
	QLabel::setText(orgText);
	int size = getWidth();
	if (size <= maxWidth)
		return;
	int p1 = 0;
	int p2 = orgText.length() - 1;
	QString strunc;
	int mid = -1;
	//int width = -1;
	int prevmid = -1;
	for (;;)
	{
		mid = (p1 + p2) / 2;
		if (prevmid == mid)
			break;
		prevmid = mid;

		strunc = orgText.mid(0, mid) + "...";
		QLabel::setText(strunc);
		int width = getWidth();
		if (width >= maxWidth)
			p2 = mid;
		else
			p1 = mid;
	}
}

