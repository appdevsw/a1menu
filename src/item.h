#ifndef ITEM_H
#define ITEM_H

#include <QWidget>
#include <QLabel>
#include <QListWidgetItem>
#include "itemlabel.h"
#include <QMenu>

#pragma pack(push, 1)

namespace Ui
{
class Item;
}

class MainWindow;
class CategoryLoader;
class ItemList;

class Item: public QWidget
{
Q_OBJECT

friend class Loader;
public:

	static const int ITEM_TYPE_APP=1;
	static const int ITEM_TYPE_CATEGORY=2;
	static const int ITEM_TYPE_PLACE=4;
	static const int ITEM_TYPE_STATIC=8;


	struct Order
	{
		const static int ord_default = 10;
		const static int ord_place_gtk = 20;
		const static int BY_TEXT=0;
		const static int BY_RECENT=1;
	};

	struct uidata
	{
		QFrame * labelFrame = NULL;
		ItemLabel title;
		ItemLabel comment;
		QLabel * icon = NULL;
		QFrame * frame = NULL;
		bool hidden=false;
	} gui;

	struct desktop_record
	{
		QString fname = "";
		QString iconName = "";
		QString command = "";
		QString title = "";
		QString loctitle = "";
		QString genname = "";
		QString locgenname = "";
		QString comment = "";
		QString loccomment = "";
		std::vector<QString> vcategories;
	} rec;

	struct loader_data
	{
		int type=0;
		bool desktop=false;
		int dupId=0;
		int serial=0;
		int catSerial=0;
		int hide=0;
		QString dirFile;
	} load;



	explicit Item(QWidget *parent = 0, int sort = Order::ord_default, bool separator = false);
	virtual ~Item();
	bool event(QEvent * event);
	void addToGuiList();
	void addToBaseList(ItemList * ilist);
	bool isAppItem();
	bool isPlace();
	bool isFavorite();
	bool isSeparator();
	int isRecent();
	int row();
	QString searchText();
	QString sortText(int mode = Order::BY_TEXT);
	void runApplication();
	QListWidgetItem * widget();
	Item * category();
	QWidget * frame();
	ItemList * list();
	void setSeparatorStyle();
	void setIconPath(QString ipath);
	QString getIconPath();
	void setTooltip();
	void setItemObjectName();
	void showContextMenu();
	const QPixmap * getIconPixmap();

private:
	void changeLayout(QLayout * l);
	QString iconPath;
	ItemList * itemList = NULL;
	Ui::Item *ui;
	unsigned sort = Order::ord_default;
	QListWidgetItem * listWidgetItem = NULL;
	Item * categoryItem = NULL;
	int recentSerial = 0;
	bool separator;

	void dragAndDropAction(QEvent * event);

private slots:
	void contextMenuAction(QAction * action);

};

#pragma pack(pop)

#endif // ITEM_H
