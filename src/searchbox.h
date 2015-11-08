#ifndef QT_MENU_SEARCHBOX_H_
#define QT_MENU_SEARCHBOX_H_

#include <QLineEdit>
#include <QIcon>

class SearchBox;

class SearchBoxCallback
{
public:
	SearchBoxCallback();
	virtual ~SearchBoxCallback();
	virtual void onSearchBoxCallback(SearchBox * sb, QEvent * event, QString txt)=0;
};

class SearchBox: public QLineEdit
{
public:
	SearchBox(QWidget * parent = 0);
	void paintEvent(QPaintEvent * event);
	void setIcons(QString isearch, QString iclear,int isize);
	virtual void mousePressEvent(QMouseEvent * event);
	virtual void mouseMoveEvent(QMouseEvent * event);
	virtual bool event(QEvent * event);
	void setCallback(SearchBoxCallback * callback);
private:
	int isize = 0;
	QPixmap pixsearch;
	QPixmap pixclear;
	int space1 = 5;
	int space2 = 5;
	QString prevText;
	SearchBoxCallback * callback = NULL;
};

#endif /* QT_MENU_SEARCHBOX_H_ */
