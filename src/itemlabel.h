#ifndef QT_LNK_ITEMLABEL_H_
#define QT_LNK_ITEMLABEL_H_

#include <QString>
#include <QLabel>
#include <QPaintEvent>

class Item;

class ItemLabel: public QLabel
{
public:
	ItemLabel();
	virtual ~ItemLabel();
	QString text();
	void setText(const QString &);
	void paintEvent(QPaintEvent * event);
	void enableTrimMode(bool enable);
private:
	int getWidth();
	void trimToSize(int maxWidth);
	QString orgText;
	bool trimMode = true;
	int prevPaintWidth=-1;
};

#endif /* QT_LNK_ITEMLABEL_H_ */
