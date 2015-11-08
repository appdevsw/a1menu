#include "ctx.h"
#include "item.h"
#include "iconloader.h"
#include "categoryloader.h"
#include "ui_item.h"
#include "mainwindow.h"
#include "itemproperties.h"
#include "itemlabel.h"
#include "itemlist.h"
#include "toolkit.h"
#include "config.h"
#include "searchbox.h"
#include <assert.h>
#include <QIcon>
#include <QProcess>
#include <QPaintEvent>
#include <QUrl>
#include <QList>
#include <QStringBuilder>

using namespace ctx;

Item::Item(QWidget *parent, int sort, bool separator) :
		QWidget(parent), ui(new Ui::Item)
{
	ui->setupUi(this);
	gui.icon = ui->icon;
	gui.frame = ui->frameWidget;

	ui->icon->setText("");

	gui.labelFrame = ui->labelFrame;
	gui.labelFrame->layout()->addWidget(&gui.title);
	gui.labelFrame->layout()->addWidget(&gui.comment);
	gui.labelFrame->layout()->setMargin(0);

	this->sort = sort;
	this->separator = separator;

	this->setContextMenuPolicy(Qt::CustomContextMenu);
}

Item::~Item()
{
	//qDebug("Item destructor %s",QS(rec.title));
	delete ui;
}

void Item::changeLayout(QLayout * l)
{
	gui.frame->layout()->removeWidget(gui.icon);
	gui.frame->layout()->removeWidget(gui.labelFrame);
	delete gui.frame->layout();
	l->setMargin(0);
	l->addWidget(gui.icon);
	l->addWidget(gui.labelFrame);
	gui.frame->setLayout(l);
}

void Item::setTooltip()
{
	QString tooltip;
	if ((CFGBOOL("app_enable_tooltips") && isAppItem()) || (CFGBOOL("category_enable_tooltips") && !isAppItem()))

	{
		QString labTitle = gui.title.text();
		QString labComment = gui.comment.text();
		QString genname = rec.locgenname == "" ? rec.genname : rec.locgenname;
		QString loctitle = rec.loctitle == "" ? rec.title : rec.loctitle;

		tooltip = "<b>" % labTitle % "</b>";

		if (labTitle != genname && genname != "")
			tooltip = tooltip % "<br>" % genname;
		if (labTitle != loctitle && loctitle != "")
			tooltip = tooltip % "<br>" % loctitle;
		if (labComment != "")
			tooltip = tooltip % "<br><i>" % labComment % "</i>";

		QString f1 = "<font size=\"2\">";
		QString f2 = "</font>";
		QString br = "<br>";
#define dispattr(lab,txt) tooltip=tooltip % br % f1 % "<br><b>" % (lab) % "</b> " % (txt) % f2,br=""

		if (rec.command != "")
			dispattr(tr("Command:"), rec.command);
		if (rec.fname != "")
			dispattr((isAppItem() ? tr("Launcher:") : tr("Directory file:")), rec.fname);
		if (rec.iconName != "")
			dispattr(tr("Icon:"), rec.iconName % ": " % iconPath);
		if (categoryItem != NULL)
			dispattr(Config::getVTrans("Category:"), categoryItem->rec.title);

		gui.frame->setToolTip(tooltip);
	}

}

void Item::addToGuiList()
{
	//qDebug("add gui item %s",QS(rec.title));
	gui.labelFrame->setContentsMargins(0, 0, 0, 0);
	QString loctitle = rec.loctitle;
	if (loctitle == "")
	{
		loctitle = rec.loctitle = rec.title;
	}

	QString genname = rec.locgenname == "" ? rec.genname : rec.locgenname;
	QString labTitle;
	if (CFGBOOL("use_generic_names"))
		labTitle = genname;
	else
		labTitle = loctitle;

	if (labTitle.length() == 0)
		labTitle = loctitle;
	if (labTitle.length() == 0)
		labTitle = genname;
	if (labTitle.length() == 0)
		labTitle = QString(rec.fname);

	QString labComment = rec.loccomment != "" ? rec.loccomment : rec.comment;

	int showText, showIcon;
	gui.title.hide();
	gui.comment.hide();

	gui.title.setText(labTitle);
	gui.comment.setText(labComment);

	if (isAppItem())
	{
		showText = CFGBOOL("app_item_show_text");
		showIcon = CFGBOOL("app_item_show_icon");
		if (CFGBOOL("app_item_with_comment"))
		{
			if (labComment != "" && showText)
			{
				gui.comment.show();
			}
		}
	} else
	{
		showText = CFGBOOL("category_item_show_text");
		showIcon = CFGBOOL("category_item_show_icon");
		gui.comment.hide();
	}
	if (!showIcon)
		showText = true;

	if (showText)
		gui.title.show();

	enum VType
	{
		LIST, ICONS, COMPACT,
	};

	int isize, h, w, lmargin = 0;
	QString vt;
	if (isAppItem())
	{
		isize = CFG("app_icon_size").toInt();
		h = CFG("app_item_height").toInt();
		w = CFG("app_item_width").toInt();
		//iview = CFG("app_view_type") == "icons";
		vt = CFG("app_view_type");
		if (isPlace())
		{
			isize = CFG("place_icon_size").toInt();
			h = CFG("place_item_height").toInt();
		}

	} else
	{
		isize = CFG("category_icon_size").toInt();
		h = CFG("category_item_height").toInt();
		w = CFG("category_item_width").toInt();
		//iview = CFG("category_view_type") == "icons";
		vt = CFG("category_view_type");
	}

	VType vtype = vt == "list" ? LIST : vt == "compact" ? COMPACT : ICONS;

	listWidgetItem = new QListWidgetItem();
	QSize widgetSize(w, h);
	listWidgetItem->setSizeHint(widgetSize);

	int wmax;
	QLayout * xlayout;
	switch (vtype)
	{

	case VType::COMPACT:

		listWidgetItem->setText("");
		wmax = 2000;
		lmargin = 2;
		gui.icon->setFixedSize(isize, isize);
		gui.labelFrame->setBaseSize(wmax, h);
		xlayout = new QHBoxLayout();
		xlayout->setGeometry(QRect(0, 0, wmax, h));
		changeLayout(xlayout);
		gui.icon->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

		//gui.title.setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
		//gui.comment.setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

		if (gui.comment.isHidden())
		{
			gui.title.setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
			//gui.comment.setAlignment(Qt::AlignLeft | Qt::AlignTop);
		} else
		{
			gui.title.setAlignment(Qt::AlignLeft | Qt::AlignBottom);
			gui.comment.setAlignment(Qt::AlignLeft | Qt::AlignTop);
		}

		gui.title.enableTrimMode(true);
		break;

	case VType::ICONS:

		wmax = w;
		gui.icon->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
		gui.icon->setMinimumSize(0, 0);
		gui.icon->setBaseSize(isize, isize);
		gui.labelFrame->setBaseSize(w, h);

		xlayout = new QVBoxLayout();
		changeLayout(xlayout);
		gui.icon->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
		gui.title.setAlignment(Qt::AlignHCenter | Qt::AlignTop);
		gui.comment.setAlignment(Qt::AlignHCenter | Qt::AlignTop);

		listWidgetItem->setFont(QFont("", 2000));
		listWidgetItem->setText(" ");
		gui.title.enableTrimMode(false);

		break;

	case VType::LIST:

		listWidgetItem->setText("");
		wmax = 2000;
		lmargin = 2;
		gui.icon->setFixedSize(isize, isize);
		gui.labelFrame->setBaseSize(wmax, h);
		xlayout = new QHBoxLayout();
		xlayout->setGeometry(QRect(0, 0, wmax, h));
		changeLayout(xlayout);
		gui.icon->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

		//gui.title.setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
		//gui.comment.setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

		if (gui.comment.isHidden())
		{
			gui.title.setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
			//gui.comment.setAlignment(Qt::AlignLeft | Qt::AlignTop);
		} else
		{
			gui.title.setAlignment(Qt::AlignLeft | Qt::AlignBottom);
			gui.comment.setAlignment(Qt::AlignLeft | Qt::AlignTop);
		}

		gui.title.enableTrimMode(true);

		break;
	}

	if (!showIcon)
	{
		gui.icon->hide();
		if (vtype != ICONS)
		{
			gui.frame->layout()->setContentsMargins(3, 0, 0, 0);
		} else
		{
			gui.title.setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
		}
	}

	gui.frame->setGeometry(lmargin, 0, wmax, h);
	gui.labelFrame->setGeometry(0, 0, wmax, h);
	setItemObjectName();
	setSeparatorStyle();
	itemList->uiList()->addItem(listWidgetItem);
	itemList->uiList()->setItemWidget(listWidgetItem, this);

}

bool Item::isSeparator()
{
	return separator;
}

void Item::setSeparatorStyle()
{
	if (separator)
	{
		bool isIconMode = CFG("category_view_type") == "icons";
		listWidgetItem->setHidden(isIconMode);
		gui.icon->setPixmap(QPixmap());
		gui.title.setText("");
		gui.comment.setText("");
		rec.title = "";
		listWidgetItem->setFlags(Qt::NoItemFlags);
		//QSize s(2000,10);
		//listWidgetItem->setSizeHint(s);
		QFrame * f = gui.frame;
		int h = 10;
		f->setMaximumHeight(h);
		//f->setMinimumHeight(6);
		//f->setBaseSize(QSize(f->width(),h));
		int w = this->list()->uiList()->width();
		listWidgetItem->setSizeHint(QSize(w, h));
		//f->setBaseSize(QSize(f->width(),h));
		ui->labelFrame->hide();
		f->setLineWidth(1);
		f->setFrameShape(QFrame::HLine);
		f->setFrameShadow(QFrame::Sunken);
	}

}

bool Item::event(QEvent * event)
{

//qDebug("item event %i",event->type());

	int tp = event->type();
	if (tp == QEvent::ContextMenu)
	{
		showContextMenu();
		return true;
	}

	if (tp == QEvent::Resize && isSeparator())
		setSeparatorStyle();

	dragAndDropAction(event);
	if (ctx::wnd != NULL)
	{
		ctx::wnd->onItemEvent(this, event);
		return false;
	}
	return QWidget::event(event);

}

bool Item::isAppItem()
{
	return rec.command != "";
}

bool Item::isPlace()
{
	return categoryItem == ctx::categoryPlaces;
}

QString Item::sortText(int mode)
{
	if (mode == Order::BY_RECENT)
		return QString().sprintf("%8i %p", 1000000 - this->recentSerial, this);

	QString title = rec.loctitle != "" ? rec.loctitle : rec.title;
	QString comment = rec.loccomment != "" ? rec.loccomment : rec.comment;
	if (title.length() == 0)
		title = QString(rec.fname);

	QString cmp = (title + " " + comment).toLower();

	return QString().sprintf("%8i%s %p", this->sort, QS(cmp), this);
}

QString Item::searchText()
{
	QString searchtext = rec.title;
	searchtext += " " + rec.loctitle;
	searchtext += " " + rec.comment;
	searchtext += " " + rec.loccomment;
	searchtext += " " + rec.genname;
	searchtext += " " + rec.locgenname;
	searchtext += " " + rec.command;
	searchtext += " " + rec.fname;
	return searchtext.toLower();
}

void Item::addToBaseList(ItemList * ilist)
{
	QString iconprop = ItemProperties(this).get(ItemProperties::pn.IconPath);
	if (iconprop != "")
		rec.iconName = iconprop;
	ilist->vitems().push_back(this);
	itemList = ilist;

}

int Item::row()
{
	assert(itemList!=NULL);
	return itemList->uiList()->row(listWidgetItem);
}

bool Item::isFavorite()
{
	return ItemProperties(this).get(ItemProperties::pn.InFavorites) == ParameterForm::YES;
}

int Item::isRecent()
{
	return recentSerial;
}

void Item::runApplication()
{
	Toolkit toolkit;
	static int recentCounter = 0;
	QString cmd = rec.command;
	cmd.replace("%F", "");
	cmd.replace("%U", "");
	cmd.replace("%u", "");
	cmd.replace("%c", rec.loctitle);
	if (rec.iconName != "")
		cmd.replace("%i", "--icon " + rec.iconName);
	else
		cmd.replace("%i", "");

	QString fileman = CFG("command_file_manager");
	if (fileman == "")
		fileman = "caja";
	cmd.replace("$open$", fileman);
	cmd.replace("$filemanager$", fileman);
	cmd.replace("~", QDir::homePath());
	cmd.replace("$home$", QDir::homePath());

	if (cmd.contains("$desktop$"))
	{
		QString desktopPath = Config::getDesktopPath();
		cmd.replace("$desktop$", desktopPath);
	}

	Toolkit().runCommand(cmd);
	ctx::searchBox->setText("");
	ctx::wnd->hide();
	recentSerial = ++recentCounter;

}

QListWidgetItem * Item::widget()
{
	return listWidgetItem;
}

Item * Item::category()
{
	return categoryItem;
}

QWidget * Item::frame()
{
	return gui.frame;
}

ItemList * Item::list()
{
	return itemList;
}

void Item::setIconPath(QString ipath)
{
	iconPath = ipath;
}

QString Item::getIconPath()
{
	return iconPath;
}

const QPixmap * Item::getIconPixmap()
{
	return gui.icon->pixmap();
}

void Item::showContextMenu()
{
	if (menu == NULL)
	{
		menu = new QMenu();
		menu->addAction(ctx::menuProperties);
		if (isAppItem())
		{
			ItemProperties ip(this);
			if (!isPlace())
				menu->addAction(ctx::menuAddToDesktop);
			if (ip.get(ip.pn.InFavorites) != ParameterForm::YES)
				menu->addAction(ctx::menuAddToFavorites);
			else
				menu->addAction(ctx::menuRemoveFromFavorites);
		} else
		{
			menu->addAction(ctx::menuSetDefaultCategory);
		}
		connect(menu, SIGNAL(triggered(QAction*)), this, SLOT(contextMenuAction(QAction*)));
	}
	auto pos = QCursor::pos();
//this->menu->popup(pos, NULL);
	ctx::wnd->lockListNavigation(true);
	menu->exec(pos);
	ctx::wnd->lockListNavigation(false);
	delete menu;
	menu = NULL;
}

void Item::contextMenuAction(QAction * action)
{
//qDebug("contextMenuAction %p %s", action, QS(action->text()));
	if (action->text() == ctx::menuProperties)
	{
		ctx::wnd->show();
		ctx::wnd->setShowForce(true);
		ItemProperties ip(this);
		int changed = ip.runPropertiesDialog();
		ctx::wnd->setShowForce(false);
		if (changed)
			ctx::wnd->reloadOnShow();
	}
	if (action->text() == ctx::menuAddToDesktop)
	{
		if (rec.fname != "")
		{
			QString dstPath = Config::getDesktopPath() + "/" + QFileInfo(rec.fname).fileName();
			if (QFile::copy(rec.fname, dstPath))
			{
				QFile dst(dstPath);
				dst.setPermissions(dst.permissions() | QFile::ExeOwner);
			}

		}
	}
	if (action->text() == ctx::menuAddToFavorites)
	{
		ItemProperties ip(this);
		ip.set(ip.pn.InFavorites, ParameterForm::YES);
	}
	if (action->text() == ctx::menuRemoveFromFavorites)
	{
		ItemProperties ip(this);
		ip.set(ip.pn.InFavorites, "");
	}

	if (action->text() == ctx::menuSetDefaultCategory)
	{
		ConfigMap cmap;
		cmap.load(PATH(GLOBAL_PROPERTIES));
		cmap["default_category"] = rec.title;
		cmap.save(PATH(GLOBAL_PROPERTIES));
	}

}

void Item::dragAndDropAction(QEvent * event)
{
	static QPoint startDragPos;
	QMouseEvent *me = dynamic_cast<QMouseEvent *>(event);
	if (me == NULL)
		return;
	int tp = event->type();
	if (tp == QEvent::MouseButtonPress)
		startDragPos = me->pos();

	if (tp == QEvent::MouseMove)
	{
		if (me->buttons() == Qt::LeftButton && ((me->pos() - startDragPos).manhattanLength() > 2 * QApplication::startDragDistance()))
		{

			QDrag *drag = new QDrag(this);
			QMimeData *mimeData = new QMimeData();
			Qt::DropAction action;
			if (isAppItem() && !isPlace())
			{
				action = Qt::CopyAction;
				QList<QUrl> urls;
				urls.append(QUrl("file://" + rec.fname));
				mimeData->setUrls(urls);
				//mimeData->setText("file://"+item->rec.fname);
			}
			drag->setMimeData(mimeData);
			drag->setPixmap(*this->getIconPixmap());
			drag->exec(action);
			//Qt::DropAction dropAction = drag->exec(Qt::CopyAction);
			//qDebug("drop %i", dropAction);
		}
	}

}

void Item::setItemObjectName()
{
	auto item = this;
	if (item->isSeparator())
	{
		item->setObjectName("ITEM_SEPARATOR");
		item->frame()->setObjectName("SEPARATOR");

	} else if (item->isAppItem())
	{
		item->setObjectName("APP_ITEM");
		item->frame()->setObjectName("APP_ITEM_FRAME");
		item->gui.title.setObjectName("APP_ITEM_TITLE");
		item->gui.comment.setObjectName("APP_ITEM_COMMENT");
		if (item->isPlace())
		{
			item->gui.title.setObjectName("PLACE_ITEM_TITLE");
			item->gui.comment.setObjectName("PLACE_ITEM_COMMENT");
		}
	} else
	{
		item->setObjectName("CAT_ITEM");
		item->frame()->setObjectName("CAT_ITEM_FRAME");
		item->gui.title.setObjectName("CAT_ITEM_TITLE");
		item->gui.comment.setObjectName("CAT_ITEM_COMMENT");
	}

}
