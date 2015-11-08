#include "itemlist.h"
#include "item.h"
#include "ui_itemlist.h"
#include "toolkit.h"
#include <QKeyEvent>
#include <QTime>
#include <assert.h>
#include <userevent.h>
#include "iconloader.h"
#include "categoryloader.h"
#include "ctx.h"
#include "itemproperties.h"
#include "applicationloader.h"
#include "mainwindow.h"
#include "config.h"
#include "loader.h"

using namespace std;
using namespace ctx;

ItemList::ItemList(QWidget *parent) :
		QWidget(parent), ui(new Ui::ItemList)
{
	ui->setupUi(this);
}

ItemList::~ItemList()
{
	delete ui;
}

bool ItemList::event(QEvent * event)
{
	//qDebug("ItemList event pid %i event %i", getpid(),event->type());
	//if (!this->hasFocus())
	//	return false;

	///ctx::wnd->onItemListEvent(this, event);

	//qDebug("onItemListEvent type: %d isapp %i", event->type(), this == ctx::appList);

	if (ctx::wnd->runApplicationOnKey(event))
		return false;
	int tp = event->type();
	if (tp == QEvent::ShortcutOverride || tp == QEvent::KeyPress)
	{
		auto searchBox = ctx::searchBox;
		QKeyEvent *ke = (QKeyEvent *) event;
		int key = ke->key();
		//qDebug(" key %i", key);
		if (searchBox != NULL && (((key >= 32 && key <= 128) || (key == Qt::Key_Backspace && tp != QEvent::ShortcutOverride))))
		{
			searchBox->setFocus();
			string txt = searchBox->text().toStdString();
			if (key == Qt::Key_Backspace)
			{
				if (txt.length() > 0)
					txt = txt.substr(0, txt.length() - 1);
			} else
			{
				txt += (char) tolower((char) key);
			}
			searchBox->setText(txt.c_str());
			searchBox->deselect();
			searchBox->setCursorPosition(txt.length());
			return true;
		}

	}

	return false;
}

void ItemList::fillGuiList()
{
	uiList()->clear(); //also destruct QListWidgetItem's . checked!

	for (size_t i = 0; i < items.size(); i++)
	{
		Item * it = items[i];
		it->addToGuiList();
	}

}

void ItemList::reorder(int mode)
{
	//auto t1 = ctx::clockmicro();
	//qDebug("reorder %i",mode);

	map<QString, Item *> sort;
	int pos = items.size() - 1;
	for (auto it : items)
	{
		QString key = it->sortText(mode);
		//assert(sort.find(key) == sort.end());
		sort[key] = it;
		uiList()->takeItem(pos--);
	}
	items.clear();
	//pos = 0;
	for (auto e : sort)
	{
		//qDebug("sort %i %s",mode,QS(e.first));
		Item * it = e.second;
		items.push_back(e.second);
		ui->listWidget->addItem(it->widget());
		//pos++;
		ui->listWidget->setItemWidget(it->widget(), it);
		if (it->isAppItem())
			ui->listWidget->setRowHidden(it->row(), !it->isRecent());
		else
			ui->listWidget->setRowHidden(it->row(), it->gui.hidden);
	}
	//qDebug("reorder time %lld", (ctx::clockmicro() - t1) / 1000LL);
}

void ItemList::refreshList(QString txt)
{
	Toolkit toolkit;
	vector<QString> filter;
	if (txt != "")
		toolkit.tokenize(txt, filter);
	refresh(filter);
	if (visibleCount > 0)
	{
		uiList()->setCurrentRow(visibleFirstRow);
	}
}

void ItemList::refresh(std::vector<QString>& filter)
{
	QString searchText = ctx::searchBox->text();
	QString category;
	Item * currentCategoryItem = NULL;
	bool isCategoryAll = false, isCategoryFav = false, isCategoryRecent = false;
	if (ctx::appList == this)
	{
		currentCategoryItem = ctx::catList->getSelectedItem();
		if (currentCategoryItem == NULL)
			currentCategoryItem = ctx::categoryAll;
		isCategoryAll = currentCategoryItem == ctx::categoryAll;
		isCategoryFav = currentCategoryItem == ctx::categoryFavorites;
		isCategoryRecent = currentCategoryItem == ctx::categoryRecent;
	}
	//qDebug("refresh %s",QS(currentCategoryItem->rec.title));
	visibleCount = 0;
	visibleFirstRow = -1;
	int sortMode = isCategoryRecent ? Item::Order::BY_RECENT : Item::Order::BY_TEXT;
	if (currentSortMode != sortMode)
	{
		reorder(sortMode);
		currentSortMode = sortMode;
	}
	for (size_t i = 0; i < items.size(); i++)
	{
		Item * item = items[i];
		QString txt = item->searchText();
		int found = 1;
		for (QString fil : filter)
		{
			fil = fil.toLower();
			if (!txt.contains(fil))
			{
				found = 0;
				break;
			}
		}
		bool isDesktop = currentCategoryItem->rec.title == ctx::nameCategoryDesktop;

		assert(item->category()!=NULL);
		int belongsToCategory = currentCategoryItem != NULL && (item->category() == currentCategoryItem || isCategoryAll);
		if (isCategoryRecent && item->isRecent())
			belongsToCategory = true;
		if (item->isPlace() && item->category() != currentCategoryItem)
			belongsToCategory = false;
		if (isCategoryFav && item->isFavorite())
			belongsToCategory = true;
		if (item->load.hide && !isDesktop)
			belongsToCategory = false;
		if (item->load.desktop && isDesktop)
			belongsToCategory = true;

		if (found && searchText == "" && !belongsToCategory)
			found = 0;
		if (found)
		{
			if (visibleCount++ == 0)
			{
				uiList()->setCurrentRow(i);
				visibleFirstRow = i;
			}
		}
		uiList()->setRowHidden(i, !found);
	}
}

Item * ItemList::getSelectedItem()
{
	int row = uiList()->currentRow();
	if (row < 0 || row >= (int) items.size())
		return NULL;
	return items[row];

}

Item * ItemList::addStaticCategory(QString cat, int sort)
{
	for (auto item : items)
		if (item->rec.title == cat)
		{
			return item;
		}

	bool isSeparator = cat == "";
	Item * itemCat = new Item(0, sort, isSeparator);
	itemCat->rec.title = cat;
	itemCat->rec.loctitle = Config::getVTrans(cat);

	QString propTitle = ItemProperties(itemCat).get(ItemProperties::pn.CategoryTitle);
	if (!propTitle.isEmpty())
		itemCat->rec.loctitle = propTitle;
	itemCat->addToBaseList(this);
	itemCat->load.type |= Item::ITEM_TYPE_STATIC;
	return itemCat;
}

void ItemList::clear()
{
	visibleCount = 0;
	visibleFirstRow = -1;
	prevRow = -1;
	currentSortMode = -1;
	uiList()->clear();

	for (Item * it : items)
	{
		delete it;
	}

	items.clear();

}

void ItemList::on_listWidget_currentRowChanged(int row)
{
	if (this == ctx::catList && row >= 0)
	{
		ctx::appList->refreshList("");
	}
	prevRow = row;
}

std::vector<Item *> & ItemList::vitems()
{
	return items;
}

QListWidget * ItemList::uiList()
{
	return ui->listWidget;
}
