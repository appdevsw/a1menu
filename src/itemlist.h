#ifndef ITEMLIST_H
#define ITEMLIST_H

#include <QWidget>
#include <QListWidget>
#include <vector>

//class MainWindow;
class Item;
class IconLoader;
class CategoryLoader;

namespace Ui
{
class ItemList;
}

class ItemList: public QWidget
{
Q_OBJECT

public:
	explicit ItemList(QWidget *parent = 0);
	virtual ~ItemList();
	bool event(QEvent * event);
	void fillGuiList();
	void refresh(std::vector<QString>& filter);
	Item * getSelectedItem();
	Item * addStaticCategory(QString category,int sort=10);
	void clear();
	void refreshList(QString txt);
    QListWidget * uiList();
	void reorder(int mode);
	//void setCategoryIcons();


	std::vector<Item *> & vitems();

private:
	int visibleCount=0;
	int visibleFirstRow=0;
	int prevRow=-1;
	Ui::ItemList *ui;
	std::vector<Item *> items;
	int currentSortMode=-1;
private slots:
    void on_listWidget_currentRowChanged(int currentRow);
};

#endif // ITEMLIST_H
