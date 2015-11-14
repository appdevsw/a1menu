#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QObject>
#include <QDir>
#include <QMainWindow>
#include <QMutexLocker>
#include <QMutex>
#include <QTime>
#include <QEvent>
#include <QLineEdit>
#include <QPushButton>
#include <QHBoxLayout>
#include "configmap.h"
#include <unistd.h>
#include <vector>

//#include "iconloader.h"
#include "searchbox.h"

namespace Ui
{
class MainWindow;
}

class ItemList;
class Item;

class MainWindow: public QMainWindow, public SearchBoxCallback
{
Q_OBJECT

	friend class Loader;

public:
	explicit MainWindow(QWidget *parent = 0);
	virtual ~MainWindow();
	virtual void showEvent(QShowEvent * event);
	virtual void hideEvent(QHideEvent * event);
	bool eventFilter(QObject *obj, QEvent *event);
	void onItemEvent(Item * item, QEvent * event);
	void sendShowEvent();
	void activateWindowForce();
	void init();
	void populate();
	//void refreshModel();
	//void refreshGui();
	bool runApplicationOnKey(QEvent * event);
	void onSearchBoxCallback(SearchBox * sb, QEvent * event, QString txt);
	void setShowForce(bool enable);
	void saveState(bool saveSplitter = false);
	void lockListNavigation(bool enable);
	void reloadOnShow();
	int setGtkPanelButton();
	void quitApp();
	void setStartupRow(int row = -1);
private:
	Ui::MainWindow *ui;
	void changeItemSelection(Item * item);
	void sendKey(QWidget * widget, int key);
	void moveOrResize(QEvent * event);
	void setSplitter();

	QString prevSearchText;
	ConfigMap location;
	bool showForce = false;
	bool isListNavigationLocked = false;
	bool reload = false;

	struct selection_t
	{
		int serial = 0;
	} sel;

	struct mouse_drag_data
	{
		QPoint pos;
		QPoint glbpos;
		//QPoint wpos;
		//QSize size;
		QRect rect;
		int move = 0;
		int resize = 0;
		//int left = 0;
		int splitterPos = -1;

		int lt = 0;
		int lb = 0;
		int rt = 0;
		int rb = 0;

	} drag;

	struct control_button_t
	{
		QPushButton button;
		QPoint pos;
		QPoint lastpos;
		int moving = 0;
		unsigned long long hideTime = 0;
		int wasHidden = 0;
		unsigned long long hideEventTime = 0;

	} ctrlbt;

	struct panel_button_t
	{
		QString iconPath;
		QIcon icon;
		QString label;
		int tryCount = 0;
	} panelbt;

	bool onControlButtonEvent(control_button_t * b, QEvent *event);

};

#endif // MAINWINDOW_H
