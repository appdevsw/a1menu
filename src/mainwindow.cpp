#include "mainwindow.h"
#include "ctx.h"
#include "applicationloader.h"
#include "categoryloader.h"
#include "loader.h"
#include "toolkit.h"
#include "itemproperties.h"
#include "ui_mainwindow.h"
#include <sstream>
#include "item.h"
#include <dirent.h>
#include <assert.h>
#include "iconloader.h"
#include <cmath>
#include <unistd.h>
#include <userevent.h>
#include "config.h"
#include <ctime>
#include "x11util.h"
#include "resource.h"

#include <QAction>
#include <QCoreApplication>
#include <QGraphicsOpacityEffect>
#include <QKeyEvent>
#include <QList>
#include <QListWidget>
#include <QMessageBox>
#include <QPainter>
#include <QPixmapCache>
#include <QPushButton>
#include <QScrollBar>
#include <QShortcut>
#include <QSystemTrayIcon>
#include <QTextStream>
#include <QThread>
#include <QTimer>

#undef signals

using namespace std;
using namespace ctx;

#define xDebug qDebug

MainWindow::MainWindow(QWidget *parent) :
		QMainWindow(parent), ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	ctx::searchBox = ui->lineEditSearchBox;

	ctx::buttonShutdown = ui->buttonShutdown;
	ctx::buttonLock = ui->buttonLock;
	ctx::buttonLogout = ui->buttonLogout;
	ctx::buttonSettings = ui->buttonSettings;
	ctx::buttonReload = ui->buttonReload;

	ctx::application->installEventFilter(this);
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::quitApp()
{
	ctx::searchBox->setCallback(NULL);
	ctx::application->removeEventFilter(this);
	QApplication::processEvents();
	QApplication::quit();
}

void MainWindow::setShowForce(bool enable)
{
	showForce = enable;
}

void MainWindow::lockListNavigation(bool enable)
{
	isListNavigationLocked = enable;
}

void MainWindow::reloadOnShow()
{
	reload = true;
}

void MainWindow::setStartupRow(int row)
{
	if (row >= 0)
	{
		ctx::catList->uiList()->setCurrentRow(row);
		return;
	}
	int initCategoryRow = 0;

	ConfigMap cmap;
	cmap.load(PATH(GLOBAL_PROPERTIES));
	QString defcat = cmap["default_category"];
	for (auto itc : ctx::catList->vitems())
	{
		if (defcat != "" && itc->rec.title == defcat)
		{
			initCategoryRow = itc->row();
			break;
		}
	}

	ctx::catList->uiList()->setCurrentRow(initCategoryRow);
	QScrollBar *vb = ctx::appList->uiList()->verticalScrollBar();
	vb->setValue(0);

}

void MainWindow::showEvent(QShowEvent * event)
{
	/*
	 if (reload)
	 {
	 reload = false;
	 populate();
	 }*/
	setStartupRow();
	ctx::searchBox->setFocus();

}

void MainWindow::hideEvent(QHideEvent * event)
{
	//qDebug("Hide");
	saveState();

	if (reload)
	{
		reload = false;
		populate();
	}

	ctx::searchBox->setText("");
	ctx::catList->uiList()->setCurrentRow(0);
	ctrlbt.hideTime = ctx::clockmicro();
}

void MainWindow::moveOrResize(QEvent * event)
{
	int xmargin = 10;
	int minw = 50;
	int maxw = 2000;

	int tp = event->type();
	if (tp == QEvent::Leave)
		setCursor(Qt::ArrowCursor);

	if (tp != QEvent::MouseButtonPress && tp != QEvent::MouseMove && tp != QEvent::MouseButtonRelease)
		return;
	QMouseEvent * me = (QMouseEvent *) event;
	int lbutton = (me->buttons() & Qt::LeftButton);

	int mx = me->pos().x();
	int sx = this->width();
	int hl = mx < xmargin;
	int hr = mx > sx - xmargin;

	int resize = (hl || hr);
	int move = !resize;

	if (tp == QEvent::MouseButtonPress)
	{
		drag.pos = me->pos();
		drag.glbpos = QCursor::pos();
		drag.wpos = this->pos();
		drag.size = this->size();
		drag.move = move;
		drag.resize = resize;
		drag.left = hl;
	}

	if (tp == QEvent::MouseButtonRelease)
	{
		setCursor(Qt::ArrowCursor);
		drag.move = drag.resize = 0;

		ctx::catList->reorder(Item::Order::BY_TEXT);
		ctx::catList->uiList()->setCurrentRow(0);
		ctx::appList->refreshList("");

		saveState();
	}

//qDebug("move %i resize %i obj size %i %i",drag.move,drag.resize,sx,sy);

	if (resize || drag.resize)
	{
		if (hl)
			setCursor(Qt::SizeFDiagCursor);
		else
			setCursor(Qt::SizeBDiagCursor);
	} else if (move || drag.move)
		setCursor(Qt::OpenHandCursor);
	else if (drag.move + drag.resize == 0)
		setCursor(Qt::ArrowCursor);

	if (tp == QEvent::MouseMove && lbutton)
	{
		if (drag.resize)
		{
			QPoint diff = QCursor::pos() - drag.glbpos;
			int yup = drag.pos.y() < drag.size.height() / 2;
			QSize s = drag.size;
			int newx = s.width() + (drag.left ? -1 : 1) * diff.x();
			int newy = s.height() + (yup ? -1 : 1) * diff.y();
			if (newx < minw || newx > maxw)
				return;
			if (newy < minw || newy > maxw)
				return;

			int px = drag.wpos.x() + (drag.left ? 1 : 0) * diff.x();
			int py = drag.wpos.y() + (yup ? 1 : 0) * diff.y();
			//qDebug("resize %i %i px/py %i %i",diff.x(),diff.y(),px,py);
			this->setGeometry(px, py, newx, newy);
		}

		if (drag.move)
		{
			QPoint diff = me->pos() - drag.pos;
			QPoint newpos = this->pos() + diff;
			this->move(newpos);
		}

	}

}

bool MainWindow::onControlButtonEvent(control_button_t * cb, QEvent *event)
{
	int tp = event->type();
	//qDebug("onControlButtonEvent %i", tp);
	QMouseEvent * me = dynamic_cast<QMouseEvent *>(event);

	if (tp == QEvent::MouseButtonDblClick && (me->button() & Qt::RightButton))
	{
		quitApp();
	}
	if (tp == QEvent::MouseButtonPress)
	{
		int ish = isHidden();
		auto elapsedMs = (ctx::clockmicro() - cb->hideTime) / 1000LL;
		//qDebug("hide from %lld ms", elapsedMs);
		cb->wasHidden = elapsedMs > 20;
		if (me->buttons() & Qt::LeftButton)
		{
			cb->lastpos = cb->pos = QCursor::pos();
		}

		if ((me->button() & Qt::LeftButton) && !cb->moving)
		{
			if (ish)
				//if (isHidden())
				//if (cb->wasHidden)
				activateWindowForce();
			else
			{
				hide();
			}
			return true;
		}

	}

	//show or hide main window
	if (tp == QEvent::MouseButtonRelease)
	{

		/*
		 if ((me->button() & Qt::LeftButton) && !cb->moving)
		 {
		 //if(ish)
		 if (isHidden())
		 //if (cb->wasHidden)
		 activateWindowForce();
		 else
		 {
		 hide();
		 }
		 return true;
		 }*/

		cb->moving = 0;
		cb->button.setCursor(Qt::ArrowCursor);
		cb->button.adjustSize();

	}

	// move control button
	if (tp == QEvent::MouseMove)
	{
		if (me->buttons() & Qt::LeftButton)
		{
			QPoint npos = QCursor::pos();
			if ((npos - cb->pos).manhattanLength() > QApplication::startDragDistance())
				cb->moving = 1;
			if (cb->moving)
			{
				cb->button.setCursor(Qt::OpenHandCursor);
				//auto size = cb->button.size();
				auto bpos = cb->button.pos();
				QPoint diff = npos - cb->lastpos;
				auto bnpos = bpos + diff;
				if (bnpos.x() < 0)
					bnpos.setX(0);
				if (bnpos.y() < 0)
					bnpos.setY(0);
				cb->button.move(bnpos);
				auto bdiff = cb->button.pos() - bpos;
				cb->lastpos = cb->lastpos + bdiff;
			}
			return true;
		}
	}

	return false;
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
	int tp = event->type();
	//qDebug("eventFilter pid %i event %i", getpid(),tp);

	if (obj == this && tp == QEvent::Hide)
		ctrlbt.hideEventTime = ctx::clockmicro();
	if (obj == &this->ctrlbt.button //
	&& tp == QEvent::MouseButtonPress //
	&& (((QMouseEvent*) event)->buttons() & Qt::LeftButton) //
			&& (ctx::clockmicro() - ctrlbt.hideEventTime) < 20 * 1000)
	{
		this->show();
	}

	/*
	 if (tp == 2 || tp == 3)
	 qDebug("eventFilter pid %i event %i", getpid(), tp);
	 if (obj == this && (tp == 17 || tp == 18))
	 qDebug("eventFilter pid %i event %i", getpid(), tp);
	 */

	if (tp == QEvent::MouseButtonRelease)
	{
		//save splitter state

		//if (obj == ui->splitter)
		int splitpos = ui->splitter->sizes()[0];
		if (drag.splitterPos != splitpos)
		{
			drag.splitterPos = splitpos;
			ctx::catList->reorder(Item::Order::BY_TEXT);
			ctx::catList->uiList()->setCurrentRow(0);
			ctx::appList->refreshList("");
			//qDebug("reload split");
		}

		saveState(true);

	}

	if (tp == QEvent::WindowDeactivate && obj == this)
	{
		if (!showForce)
		{
			hide();
		}
	}

	if (obj == this->ui->frameTopBar)
	{
		moveOrResize(event);
	}

	if (obj == &this->ctrlbt.button)
	{
		bool res = onControlButtonEvent(&ctrlbt, event);
		if (res)
			return true;
	}

	if (tp == QEvent::DeferredDelete)
	{
		auto t = dynamic_cast<QThread*>(obj);
		if (t != NULL)
		{
			//qDebug("eventFilter pid %i event %i", getpid(), tp);
			//t->wait();
			t->quit();

			//delete t;
			return false;
		}
	}

	UserEvent * ue = dynamic_cast<UserEvent *>(event);

	if (ue != NULL)
	{
		Toolkit toolkit;
		vector<QString> filter;

		if(tp==UserEventType::BTN_RELOAD)
		{
			tp=UserEventType::IPC_RELOAD;
			ue->custom.clearCache=true;
		}

		switch (tp)
		{

		case UserEventType::APPLET_SHOW:
			if (this->isHidden())
			{
				activateWindowForce();

			} else
				hide();
			return true;
			break;
		case UserEventType::CONFIG_DIALOG:
		{

			Config::runDialogProcess();

			return true;
			break;

		}
		case UserEventType::SYNCHRONIZE_MENU:
			if (sel.serial == ue->custom.serial)
			{
				changeItemSelection(ue->custom.item);
				return true;
			}
			break;

		case UserEventType::SET_PANEL_BUTTON:
			setGtkPanelButton();
			return true;
			break;

		case UserEventType::LOADER:
			if (ctx::loader != NULL)
				ctx::loader->onLoaderEvent(ue);
			return true;
			break;

		case UserEventType::IPC_RELOAD:

			if (ue->custom.clearCache)
				toolkit.removeRecursively(PATH(CACHE));

			//qDebug("event loop received IPC_EVENT_RELOAD");
			ctx::cfgorg.setCheckExistence(false);
			ctx::cfgmod.setCheckExistence(false);
			ctx::cfgorg.load(PATH(CFG_FILE));
			Config::inheritIncludes(ctx::cfgorg, ctx::cfgmod);
			ctx::cfgorg.setCheckExistence(true);
			ctx::cfgmod.setCheckExistence(true);
			populate();
			auto it = ctx::catList->getSelectedItem();
			if (it == NULL)
				ctx::catList->uiList()->setCurrentRow(0);
			ctx::searchBox->setFocus();
			return true;
			break;

		}
	}

	return false;
}

void MainWindow::sendKey(QWidget * widget, int key)
{
	QKeyEvent *kev = new QKeyEvent(QEvent::KeyPress, key, Qt::NoModifier);
	QCoreApplication::postEvent(widget, kev);
	kev = new QKeyEvent(QEvent::KeyRelease, key, Qt::NoModifier);
	QCoreApplication::postEvent(widget, kev);
}

void MainWindow::onItemEvent(Item * item, QEvent * event)
{
	int tp = event->type();
//qDebug("Item Event: %s type %i", QS(item->rec.title), tp);

	ItemList * ilist = item->list();
	bool isApp = ilist == ctx::appList;
	bool isCategory = !isApp;
	if (ilist == NULL)
		return;

	QMouseEvent *me = dynamic_cast<QMouseEvent *>(event);

	if (!isListNavigationLocked && !item->isSeparator())
	{
		int change = 0;
		if (tp == QEvent::MouseButtonRelease)
		{
			change = 1;
			if (isApp && me->button() == Qt::LeftButton)
			{
				item->runApplication();
				return;
			}
		}

		if (isCategory)
		{

			if (tp == QEvent::Leave)
			{
				sel.serial++;
			}

			if (tp == QEvent::Enter && item->row() != ctx::catList->uiList()->currentRow())
			{
				sel.serial++;
				UserEvent* dt = new UserEvent(UserEventType::SYNCHRONIZE_MENU);
				dt->custom.serial = sel.serial;
				dt->custom.item = item;
				dt->postLater(CFG("menu_sync_delay_ms").toInt());
			}
		} else
		//if(isApp)
		{

			if (tp == QEvent::Enter)
				change = 1;
		}
		if (change)
		{
			changeItemSelection(item);
		}
	}
}

void MainWindow::changeItemSelection(Item * item)
{
	ItemList * al = item->list();
	assert(al!=NULL);
	al->uiList()->setCurrentRow(item->row());
}

void MainWindow::onSearchBoxCallback(SearchBox * sb, QEvent * event, QString txt)
{
	if (runApplicationOnKey(event))
		return;
	QListWidget * wlist = ctx::appList->uiList();
	if (txt != prevSearchText)
	{

		if (txt != "")
			ui->frameCat->hide();
		else
			ui->frameCat->show();

		ctx::appList->refreshList(txt);
	}
	prevSearchText = txt;

	int jumpkeys[] = { Qt::Key_Up, Qt::Key_Down, Qt::Key_PageDown, Qt::Key_PageUp };
	if (event->type() == QEvent::KeyPress)
	{
		QKeyEvent *ke = (QKeyEvent *) event;
		int key = ke->key();
		for (auto k : jumpkeys)
			if (key == k)
			{
				wlist->setFocus();
				sendKey(wlist, key);
				break;
			}
		if (key == Qt::Key_Escape)
		{
			if (txt != "")
				ctx::searchBox->setText("");
			else
				hide();
		}
	}
}

bool MainWindow::runApplicationOnKey(QEvent * event)
{
	if (event->type() == QEvent::KeyPress)
	{
		QKeyEvent *ke = (QKeyEvent *) event;
		int key = ke->key();
		if (key == Qt::Key_Return)
		{
			int row = ctx::appList->uiList()->currentRow();
			if (row >= 0)
			{
				auto it = ctx::appList->vitems()[row];
				if (!it->widget()->isHidden())
					it->runApplication();
				return true;
			}

		}
	}
	return false;
}

void MainWindow::saveState(bool saveSplitter)
{
	location["x"] = QString::number(this->pos().x());
	location["y"] = QString::number(this->pos().y());
	location["w"] = QString::number(this->width());
	location["h"] = QString::number(this->height());

	if (saveSplitter)
	{
		auto qsizes = ui->splitter->sizes();
		int scat = qsizes[0];
		int sapp = qsizes[1];
		if (scat > 0)
		{
			//qDebug("save splitter %i %i win x %i", scat, sapp, this->width());
			if (ui->splitter->widget(0) != ui->frameCat)
				swap(scat, sapp);
			location["splitcat"] = QString::number(scat);
			location["splitapp"] = QString::number(sapp);
		}
	}
	location.save(PATH(LOCATION));

}

void MainWindow::setSplitter()
{
	if (ui->frameCat != ui->splitter->widget(CFGBOOL("categories_on_the_right") ? 1 : 0))
		ui->splitter->insertWidget(0, ui->splitter->widget(1)); //swap splitter children

	int scat = location["splitcat"].toInt();
	int sapp = location["splitapp"].toInt();
	if (ui->splitter->widget(0) != ui->frameCat)
		swap(scat, sapp);
	QList<int> qsizes;
	qsizes.append(scat);
	qsizes.append(sapp);
	ui->splitter->setSizes(qsizes);
	ui->splitter->setStretchFactor(0, 0);
	ui->splitter->setStretchFactor(1, 1);
	drag.splitterPos = ui->splitter->sizes()[0];

}

void MainWindow::sendShowEvent()
{
	UserEvent * ue = new UserEvent(UserEventType::APPLET_SHOW);
	QCoreApplication::postEvent(this, ue);
}

void MainWindow::activateWindowForce()
{
	this->show();
	X11Util t;
	t.setForegroundWindow(this->winId());
	t.setForegroundWindow(this->winId());
//yes! twice!

}

int MainWindow::setGtkPanelButton()
{
	if (ctx::standaloneMode)
	{
		ctrlbt.button.setIcon(panelbt.icon);
		ctrlbt.button.setText(panelbt.label);
		ctrlbt.button.adjustSize();
	} else
	{
		extern void panel_set_menu_button(const char * label, const char * iconPath);

		if (ctx::thread_data.isGtkInitialized == 0)
		{
			if (++panelbt.tryCount % 10 == 0)
				qDebug("Waiting for GTK thread... Did you start the menu from the command line without `%s` option ?!",
						QS(ctx::procParRunStandalone));
			(new UserEvent(UserEventType::SET_PANEL_BUTTON))->postLater(500);
			return -1;
		}

		string blab = panelbt.label.toStdString();
		string bic = panelbt.iconPath.toStdString();
		qDebug("set panel button:init %i label: %s icon: %s", ctx::thread_data.isGtkInitialized, blab.c_str(), bic.c_str());
		panel_set_menu_button(blab.c_str(), bic.c_str());
		qDebug("set panel button done");

	}
	return 0;
}

void MainWindow::init()
{
	assert(ctx::loader!=NULL);

	QPixmapCache::setCacheLimit(0);

	location.setCheckExistence(false);
	location["x"] = "0";
	location["y"] = "0";
	location["w"] = "480";
	location["h"] = "600";
	location["splitcat"] = "149";
	location["splitapp"] = "312";
	location.setCheckExistence(true);
	assert(ctx::appList==NULL);
	ctx::appList = ui->applications;
	ctx::catList = ui->categories;
	ctx::palorglist = ctx::appList->palette();
	this->setWindowFlags(Qt::Popup | Qt::SplashScreen);
	ctx::searchBox->setCallback(this);
	if (ctx::standaloneMode && !ctx::thread_data.hideCtrlButton)
	{
		ctrlbt.button.setWindowFlags(Qt::SplashScreen | Qt::FramelessWindowHint);
		ctrlbt.button.setText(tr("Menu"));
		ctrlbt.button.setStyleSheet(""
				"QPushButton{border:1px solid darkgray;outline: none;margin: 2px;padding: 2px;} "
				"QPushButton:focus{outline: none;}");
		ctrlbt.button.setToolTip(
				tr("Mouse click - show or hide menu.\nMouse drag - change button location.\nRight double click - quit menu."));
		ctrlbt.button.show();
		ctrlbt.button.move(0, 0);

	} else
	{
		ctrlbt.button.hide();
	}

	this->setObjectName("WINDOW");
	ui->centralWidget->setObjectName("WINDOW_WIDGET");
	ui->frame->setObjectName("WINDOW_FRAME");
	ctx::appList->uiList()->setObjectName("APP_LIST");
	ui->frameAppPanel->setObjectName("APP_PANEL_FRAME");
	ui->frameAppList->setObjectName("APP_LIST_FRAME");
	ctx::catList->uiList()->setObjectName("CAT_LIST");
	ui->frameCat->setObjectName("CAT_LIST_FRAME");
	ctx::searchBox->setObjectName("SEARCHBOX");
	ui->frameSearch->setObjectName("SEARCHBOX_FRAME");
	ui->splitter->setObjectName("SPLITTER");
	ui->frameSplit->setObjectName("SPLITTER_FRAME");
	ui->frameTopBar->setObjectName("TOP_BAR");
	ui->frameTool->setObjectName("TOOLBAR");
	for (auto b : ui->frameTool->children())
		((ToolButton*) b)->setObjectName("TOOL_BUTTON");

	ctx::loader->cliLoadEnv();	//wait

	ctx::palorg = palette();
	Config::initTranslation();
	ctx::cfgorg.load(PATH(CFG_FILE));
	ctx::cfgorg.setCheckExistence(true);
	Config::inheritIncludes(ctx::cfgorg, ctx::cfgmod);

	populate();

	if (!ctx::thread_data.inthread)
		activateWindowForce();
}

void MainWindow::populate()
{
	if (ctx::loader == NULL)
	{
		ctx::thread_data.tstart = ctx::clockmicro();
		ctx::loader = new Loader();
		ctx::loader->initServer();
	}

	ctx::cfgorg.setCheckExistence(false);
	ctx::cfgmod.setCheckExistence(false);
	Config::inheritIncludes(ctx::cfgorg, ctx::cfgmod);
	ctx::cfgorg.setCheckExistence(true);
	ctx::cfgmod.setCheckExistence(true);

	ctx::appList->clear();
	ctx::catList->clear();

	int pos = Item::Order::ord_default;

#define addcat(xobj,xname,xpos,xicon) \
	xobj = ctx::catList->addStaticCategory(xname, xpos); \
	xobj->setIconPath(CFG(xicon))

	addcat(ctx::categoryAll, ctx::nameCategoryAll, pos - 5, "icon_category_all");
	addcat(ctx::categoryFavorites, ctx::nameCategoryFavorites, pos - 4, "icon_category_favorites");
	addcat(ctx::categoryRecent, ctx::nameCategoryRecent, pos - 3, "icon_category_recent");
	addcat(ctx::categoryPlaces, ctx::nameCategoryPlaces, pos - 2, "icon_category_places");
	ctx::catList->addStaticCategory("", pos - 1);
	addcat(ctx::categoryOther, ctx::nameCategoryOther, pos + 999, "icon_category_other");

	ctx::buttonShutdown->command = CFG("command_shutdown");
	ctx::buttonShutdown->setToolTip(tr("Shutdown"));

	ctx::buttonLock->command = CFG("command_lock_screen");
	ctx::buttonLock->setToolTip(tr("Lock screen"));

	ctx::buttonLogout->command = CFG("command_logout");
	ctx::buttonLogout->setToolTip(tr("User logout"));

	ctx::buttonSettings->setUserEvent(UserEventType::CONFIG_DIALOG);
	ctx::buttonSettings->setToolTip(tr("Preferences"));

	ctx::buttonReload->setUserEvent(UserEventType::BTN_RELOAD);
	ctx::buttonReload->setToolTip(tr("Reload menu"));
	ctx::buttonReload->hide();


	panelbt.label = CFG("menu_label");
	if (panelbt.label == "" && panelbt.iconPath == "")
		panelbt.label = tr("Menu");
	panelbt.tryCount = 0;

	QFileInfo check(PATH(LOCATION));
	if (check.exists())
		location.load(PATH(LOCATION));
	this->setGeometry(location["x"].toInt(), location["y"].toInt(), location["w"].toInt(), location["h"].toInt());
	setSplitter();
	ctx::searchBox->setText("");

	QPalette pal = ctx::catList->palette();
	pal.setColor(QPalette::Base, pal.color(QPalette::Window));
	pal.setColor(QPalette::Background, pal.color(QPalette::Window));

	if (CFG("app_view_type") == "list")
	{
		ctx::appList->uiList()->setViewMode(QListView::ListMode);
		ctx::appList->uiList()->setWrapping(false);
	} else
	{
		ctx::appList->uiList()->setViewMode(QListView::IconMode);
		ctx::appList->uiList()->setWrapping(true);
	}
	if (CFG("category_view_type") == "list")
	{
		ctx::catList->uiList()->setViewMode(QListView::ListMode);
		ctx::catList->uiList()->setWrapping(false);
	} else
	{
		ctx::catList->uiList()->setViewMode(QListView::IconMode);
		ctx::catList->uiList()->setWrapping(true);
	}
	ctx::catList->setPalette(pal);
	ctx::appList->setPalette(pal);
	ctx::catList->uiList()->setFrameStyle(QFrame::NoFrame);
	ctx::catList->fillGuiList();

	QString style = Config::getStyleFile();
	if (style == "")
	{
		this->setAttribute(Qt::WA_TranslucentBackground, false);
		this->setStyleSheet("");

		QString bk = pal.background().color().name();
		QString bkhi = pal.background().color().darker(110).name();
		style = ""
				" QMainWindow QWidget#WINDOW_WIDGET {background-color:" + bk + "}"
				" QListWidget#CAT_LIST  {background-color:" + bk + "}"
				" QListWidget#APP_LIST  {background-color:" + bk + "}"
				" QFrame#CAT_LIST::item:!selected:hover {background-color:" + bkhi + "}"
				" QLabel#APP_ITEM_TITLE {font-weight: bold;}"
				" QLabel#APP_ITEM_COMMENT{font-style: italic;}";
	} else
	{
		this->setAttribute(Qt::WA_TranslucentBackground, true);
	}
	this->setStyleSheet(style);

	const int useLoaderEvents = 1;

	if (useLoaderEvents)
	{
		assert(ctx::loader!=NULL);
		ctx::loader->onLoaderEvent(NULL);
	} else
	{

		ctx::loader->cliLoadItems();
		ctx::loader->cliLoadCategories();
		ctx::loader->cliLoadIcons();
		ctx::loader->closeServer();

		delete ctx::loader;
		ctx::loader = NULL;
		(new UserEvent(UserEventType::SET_PANEL_BUTTON))->postLater(0);
		qDebug("loader finish time %lld", (ctx::clockmicro() - ctx::thread_data.tstart) / 1000);

	}

}
