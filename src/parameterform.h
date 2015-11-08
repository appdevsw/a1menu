#ifndef CONFIGPAR_H
#define CONFIGPAR_H

#include <QDialog>
#include <QFormLayout>
#include <QListWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLabel>

#include <QEvent>
//#include "configmap.h"

#include <vector>
#include <map>

namespace Ui
{
class ParameterForm;
}

using std::vector;
using std::map;

class ConfigItem;
class HotKeyField;
class ParameterForm;

class ParameterFormCallback
{
public:
	virtual ~ParameterFormCallback()
	{
	}
	;
	virtual bool parameterFormCallback(ParameterForm * pf, int buttonCode)=0;
};

class ParameterForm: public QDialog
{
Q_OBJECT

public:
	const static int sizeDialogWidth = 900;
	const static int sizeDialogHeight = 600;

	static QString YES;
	static QString NO;
	static QString NONE;

	class Item: public QWidget
	{
	public:
		const static int TYPE_INT = 2;
		Item(ParameterForm * pform);
		~Item();
		virtual bool event(QEvent * event);
		int setHidden(int enable);
		bool isCollapsed();
		int level();
		int isChildOf(int id);

		ConfigItem * rec = NULL;

		struct gui_t
		{
			QListWidgetItem * witem = NULL;
			QComboBox * combo = NULL;
			QLineEdit * edit = NULL;
			HotKeyField * hotkey = NULL;
			QWidget * input = NULL;
			QLayout * hbox = NULL;
			QFrame * frlab = NULL;
			QLayout * hboxlab = NULL;
			QLabel * labelColl = NULL;
			QLabel * label = NULL;
			void addToGuiList(Item * it);
			void create(Item * it);
		} gui;

		ParameterForm * pform;
		Item * parent = NULL;
		bool collapsed = false;
	};

	explicit ParameterForm(QWidget * parent = 0);
	~ParameterForm();
	bool eventFilter(QObject *obj, QEvent *event);
	void runDialog();
	void setCallback(ParameterFormCallback * callback);
	QDialogButtonBox * getButtonBox();
	int validateForm();
	map<QString, QString> getValues();
	void addItem(ConfigItem * it);
private:
	void addChildrenToGuiList(int parentid, int level = 0);
	void clear();
	void buildForm();
	void redisplay();
	void changeFontSize(QFont& font, int add);
	void fitLabels();

	QPalette palette;
	Ui::ParameterForm *ui;
	QListWidget * uiList;
	map<int, Item*> idmap;
	ParameterFormCallback * callback = NULL;
	vector<Item *> vitems;
};

#endif
