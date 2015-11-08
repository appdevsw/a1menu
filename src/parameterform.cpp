#include "parameterform.h"
#include "ui_parameterform.h"
#include "configmap.h"
#include "config.h"
#include "mainwindow.h"
#include <QListWidgetItem>
#include <QLabel>
#include <QMessageBox>
#include <QCloseEvent>
#include <assert.h>
#include <x11util.h>
#include "ctx.h"
#include "toolbutton.h"
#include "userevent.h"
#include <set>

using namespace std;

//values set in Config::
QString ParameterForm::YES;
QString ParameterForm::NO;
QString ParameterForm::NONE;

ParameterForm::ParameterForm(QWidget *parent) :
		QDialog(parent), ui(new Ui::ParameterForm)
{
	ui->setupUi(this);
	ctx::application->installEventFilter(this);

	uiList = ui->listWidget;
	auto reload = ui->buttonBox->button(QDialogButtonBox::Reset);
	reload->setText(tr("Reload menu"));
	reload->setToolTip(tr("Clear all caches, reload icons, applications and categories from the file system"));
	//qDebug("parameter form constructor %p",this);

	ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
	ui->buttonBox->button(QDialogButtonBox::Save)->setText(tr("Save"));
	ui->buttonBox->button(QDialogButtonBox::Apply)->setText(tr("Apply"));

}

ParameterForm::~ParameterForm()
{
	ctx::application->removeEventFilter(this);
	//qDebug("parameter form destructor %p",this);
	//clear();
	delete ui;
}

void ParameterForm::clear()
{
	ui->listWidget->clear();
	//for (auto it : vitems)
	//	delete it;
	vitems.clear();
	idmap.clear();
}

QDialogButtonBox * ParameterForm::getButtonBox()
{
	return ui->buttonBox;
}

bool ParameterForm::eventFilter(QObject *obj, QEvent *event)
{
	//qDebug("event %i", event->type());
	QObject * o = obj;
	for (;;)
	{
		if (o == NULL)
			return false;
		if (o == this)
			break;
		o = o->parent();
	}

	auto box = getButtonBox();
	bool isPressed = ToolButton::isButtonPressedEvent(event);
	int tp = event->type();

	if (tp == QEvent::FocusIn)
	{
		for (auto it : vitems)
			if (it->gui.input == obj)	//(obj == it->gui.edit || obj == it->gui.combo))
			{
				uiList->setCurrentItem(it->gui.witem);
			}

	}

	if (isPressed)
	{
		int bcode = 0;
		if (obj == box->button(QDialogButtonBox::Apply))
			bcode = QDialogButtonBox::Apply;
		if (obj == box->button(QDialogButtonBox::Save))
			bcode = QDialogButtonBox::Save;
		if (obj == box->button(QDialogButtonBox::Reset))
			bcode = QDialogButtonBox::Reset;
		if (obj == box->button(QDialogButtonBox::Cancel))
			bcode = QDialogButtonBox::Cancel;
		if (callback != NULL)
			return callback->parameterFormCallback(this, bcode);
	}

	//qDebug("event %i", event->type());

	if (tp == UserEventType::FIT_LABELS)
	{
		fitLabels();
		redisplay();
	}

	return false;
}

void ParameterForm::setCallback(ParameterFormCallback * callback)
{
	this->callback = callback;
}

map<QString, QString> ParameterForm::getValues()
{
	map<QString, QString> m;
	for (auto it : vitems)
		if (!it->rec->isCategory)
			m[it->rec->name] = it->rec->value;
	return m;
}

void ParameterForm::runDialog()
{
	setModal(true);
	palette = ctx::palorg;
	palette.setColor(QPalette::Background, palette.color(QPalette::Window));
	this->setPalette(palette);
	this->setStyleSheet("");

	auto f = this->font();
	f.setPointSize(11);
	this->setFont(f);

	buildForm();
	resize(sizeDialogWidth, sizeDialogHeight);
	exec();
}

void ParameterForm::addItem(ConfigItem * cfgit)
{
	Item * it = new Item(this);
	it->rec = cfgit;
	for (auto i : vitems)
		if (i->rec->id == cfgit->parentid)
		{
			it->parent = i;
			break;
		}
	vitems.push_back(it);
	it->gui.create(it);
	if (!it->rec->isCategory)
	{
		//vitemkeys[it->rec->name] = it;
		//setGuiItemValue(it->rec->name,it->rec->value);

		if (!it->rec->isCategory)
		{
			QString val = it->rec->value;
			it->rec->value = val;
			if (it->gui.combo != NULL)
			{
				QString valtr = Config::getVTrans(val);
				int idx = it->gui.combo->findText(valtr);
				if (idx >= 0)
					it->gui.combo->setCurrentIndex(idx);
			}
			if (it->gui.edit != NULL)
				it->gui.edit->setText(val);
			if (it->gui.hotkey != NULL)
				it->gui.hotkey->setText(val);
		}
	}
}

void ParameterForm::buildForm()
{
	//ui->listWidget->clear();
	addChildrenToGuiList(0);
	QApplication::postEvent(this, new UserEvent(UserEventType::FIT_LABELS));
}

void ParameterForm::addChildrenToGuiList(int parentid, int level)
{
	assert(level < 99 && "Error in tree structure");
	map<QString, Item *> vsort;
	for (auto it : vitems)
	{
		if (it->rec->parentid == parentid)
		{
			QString sort;
			sort.sprintf("%1i %5i", (it->rec->isCategory ? 1 : 0), it->rec->ord);
			sort += it->rec->label + " " + QString::number(it->rec->id);
			vsort[sort] = it;
		}
	}

	for (auto kv : vsort)
	{
		Item * it = vsort[kv.first];
		if (it->level() == 1 && it->rec->isCategory)
			it->collapsed = true;
		//qDebug("sort %s %i %s", QS(kv.first), parentid, QS(it->rec->label));
		it->gui.addToGuiList(it);
		addChildrenToGuiList(it->rec->id, level + 1);
	}
}

void ParameterForm::fitLabels()
{
	int byLevel = 1;

	if (!byLevel)
	{
		int maxw = 0;
		for (int i = 0; i <= 1; i++)
			for (auto it : vitems)
			{
				if (i == 0)
				{
					int w = it->gui.frlab->width();
					if (w > maxw)
						maxw = w;
				}
				if (i == 1)
				{
					it->gui.frlab->setMinimumWidth(maxw);
					it->gui.frlab->setMaximumWidth(maxw);
				}
			}
	}

	if (byLevel)
	{
		set<int> parents;
		for (auto it : vitems)
		{
			if (it->level() == 1)
			{
				//qDebug("parent %s %i", QS(it->rec->label),it->rec->id);
				parents.insert(it->rec->id);
			}
			//parents.insert(it->rec->parentid);
			//it->collapsed = it->level() == 1;
		}

		for (int parentid : parents)
		{
			int maxw = 0;
			for (int i = 0; i <= 1; i++)
				for (auto it : vitems)
				{
					//qDebug("is child %i", it->isChildOf(parentid));
					//if (it->rec->parentid == parentid || it->rec->id == parentid)
					if (it->isChildOf(parentid) > 0 || it->rec->id == parentid)
					{
						if (i == 0)
						{
							//qDebug("parent %i child %s",parentid,QS(it->rec->label));
							int w = it->gui.frlab->width();
							//qDebug("w %8i %s %i",w,QS(it->rec->label),it->isVisible());
							if (w > maxw)
							{
								maxw = w;
							}
						}
						if (i == 1)
						{
							it->gui.frlab->setMinimumWidth(maxw);
							it->gui.frlab->setMaximumWidth(maxw);
						}
					}
				}
		}
	}
}

#define verr(e,msg) error=msg,throw 1

int ParameterForm::validateForm()
{
	QString error = "";
	int reload = 0;
	for (int apply = 0; apply <= 1; apply++)
		for (auto it : vitems)
		{
			try
			{
				if (it->rec->isCategory)
					continue;
				QString val;
				if (it->gui.combo != NULL)
				{
					val = it->gui.combo->currentText();
					val = Config::getVTrans(val, 1);
				} else if (it->gui.hotkey != NULL)
					val = it->gui.hotkey->text();
				else
					val = it->gui.edit->text();

				if (apply)
				{
					//if (it->rec->value != val )
					//	qDebug("value changed %s %s %s",QS(it->rec->name),QS(it->rec->value),QS(val));
					if (it->rec->value != val && it->rec->reloadreq)
						reload++;
					it->rec->value = val;
					continue;
				}

				if (val == "" && it->rec->mandatory)
					verr(e, tr("Value must be entered"));

				if (it->rec->type == it->TYPE_INT)
				{
					int vali = val.toInt();
					if (val != QString::number(vali))
						verr(e, tr("Invalid integer"));
					else
					{
						if ((it->rec->rangeMin != "" && vali < it->rec->rangeMin.toInt())
								|| (it->rec->rangeMax != "" && vali > it->rec->rangeMax.toInt()))
							verr(e, tr("Range error") + " (" + it->rec->rangeMin + ":" + it->rec->rangeMax + ")");

					}
				}
				if (it->rec->validitems.size() > 0)
				{
					auto v = std::find(it->rec->validitems.begin(), it->rec->validitems.end(), val);
					if (v == it->rec->validitems.end())
						verr(e, tr("Value is not allowed"));
				}
			} catch (int e)
			{
				if (error != "")
				{
					ui->listWidget->setCurrentRow(ui->listWidget->row(it->gui.witem));
					QMessageBox::critical(0, tr("Error"), error);
					return -1;

				}
			}
		}

	return reload;
}

#undef verr

void ParameterForm::redisplay()
{
	//qDebug("redisplay");
	for (auto it : vitems)
	{
		bool iscoll = it->isCollapsed();
		if (it->rec->isCategory)
			it->gui.labelColl->setText(it->collapsed ? "+" : "-");
		it->setHidden(iscoll);
	}
}

//-------------- ParameterForm::Item -----------------------
//

ParameterForm::Item::Item(ParameterForm * pform) :
		QWidget()
{
	this->pform = pform;
	this->setParent(pform);
}

ParameterForm::Item::~Item()
{
	//qDebug("item destructor %s", QS(rec->label));
	//if (gui.hbox != NULL)
	//	delete gui.hbox;
}

void ParameterForm::changeFontSize(QFont& font, int add)
{
	if (font.pointSize() == -1)
		font.setPixelSize(font.pixelSize() + add);
	else
		font.setPointSize(font.pointSize() + add);

}

void ParameterForm::Item::gui_t::create(Item * it)
{
	//qDebug("add to gui %s", QS(it->rec->label));
	const static int sizeCombo = 150;
	const static int sizeInt = 120;
	const static int labRMargin = 10;
	const static int boxRMargin = 20;
	const static int levelMargin = 20;

	assert(hbox==NULL);

	hbox = new QHBoxLayout();
	if (it->layout() != NULL)
		delete it->layout();
	it->setLayout(hbox);
	hbox->setAlignment(Qt::AlignLeft);
	hbox->setMargin(0);

	frlab = new QFrame();
	frlab->setParent(it);

	//qDebug("qframe1 %i",frlab->width());
	hboxlab = new QHBoxLayout();
	hbox->setAlignment(Qt::AlignLeft);
	frlab->setLayout(hboxlab);
	//frlab->setStyleSheet("background-color : cyan");
	hbox->addWidget(frlab);
	hbox->setAlignment(frlab, Qt::AlignLeft);
	//hbox->setAlignment(hboxlab, Qt::AlignLeft);

	hbox->setSpacing(0);
	hbox->setContentsMargins(0, 0, boxRMargin, 0);
	hboxlab->setSpacing(0);
	hboxlab->setContentsMargins(it->level() * levelMargin, 0, labRMargin, 0);

	if (it->rec->isCategory)
	{
		labelColl = new QLabel("-");
		hboxlab->addWidget(labelColl);
		labelColl->setMaximumWidth(15);
		labelColl->setMinimumWidth(15);
	}

	QString ltxt = it->rec->label == "" ? it->rec->name : it->rec->label;
	label = new QLabel(ltxt);
	//label->setStyleSheet("background-color : yellow");
	hboxlab->addWidget(label);
	hboxlab->setAlignment(label, Qt::AlignLeft);

	if (it->rec->isCategory)
	{
		int level = it->level();
		auto font = label->font();
		font.setBold(true);
		if (level == 0)
			it->pform->changeFontSize(font, 2);
		//if (level > 1)
		//	label->setStyleSheet("color : rgb(50,50,50)");
		if (level > 1)
		{
			QPalette pal = label->palette();
			pal.setColor(QPalette::Text, QColor(60, 60, 60));
			label->setPalette(pal);
			//label->setStyleSheet("color : rgb(50,50,50)");
			//it->pform->changeFontSize(font,-1);
			font.setItalic(true);
		}
		//if (level > 2)
		//	font.setPixelSize(font.pixelSize()-5);

		label->setFont(font);
	} else
	{
		QString val = it->rec->value;
		if (it->rec->validitems.size() > 0)
		{
			input = combo = new QComboBox();
			combo->setMaximumWidth(sizeCombo);
			for (auto comboitem : it->rec->validitems)
			{
				QString val = Config::getVTrans(comboitem);
				//qDebug("combo %s %s   %s", QS(comboitem), QS(val),QS(it->rec->label));
				combo->addItem(val);
			}
			/*
			 int idx = combo->findText(val);
			 if (idx >= 0)
			 combo->setCurrentIndex(idx);
			 */
			hbox->addWidget(combo);
		} else
		{
			//edit = new QLineEdit(val);
			if (it->rec->hotkey)
			{
				input = hotkey = new HotKeyField();
				hbox->addWidget(hotkey);
			} else
				input = edit = new QLineEdit();
			if (it->rec->type == TYPE_INT)
			{
				edit->setValidator(new QIntValidator(0, (int) 1e8, it));
				edit->setMaximumWidth(sizeInt);
				//edit->setMinimumWidth(sizeInt);
				hbox->addWidget(edit);
				hbox->setAlignment(edit, Qt::AlignLeft);
			} else if (edit != NULL)
				hbox->addWidget(edit);
		}
	}

}

void ParameterForm::Item::gui_t::addToGuiList(Item * it)
{
	const static int sizeHeight = 32;
	const static int sizeRowWidth = 600;

	//create(it);

	witem = new QListWidgetItem();
	QSize widgetSize(sizeRowWidth, sizeHeight);
	witem->setSizeHint(widgetSize);
	witem->setBackground(it->pform->palette.background());

	it->pform->uiList->addItem(witem);
	it->pform->uiList->setItemWidget(witem, it);
	it->show();
}

int ParameterForm::Item::setHidden(int enable)
{
	int row = this->pform->uiList->row(gui.witem);
	int isHidden = this->pform->uiList->isRowHidden(row);
	if (enable >= 0)
		this->pform->uiList->setRowHidden(row, enable);
	return isHidden;
}

bool ParameterForm::Item::isCollapsed()
{
	bool collapsed = false;
	auto itpar = this->parent;
	while (itpar != NULL && !collapsed)
	{
		if (itpar->collapsed)
			collapsed = true;
		itpar = itpar->parent;
	}
	return collapsed;
}

bool ParameterForm::Item::event(QEvent * event)
{
	bool run = ToolButton::isButtonPressedEvent(event);
	if (run && this->rec->isCategory)
	{
		this->collapsed = !this->collapsed;
		this->pform->redisplay();
	}
	//qDebug("event %i",event->type());
	return QWidget::event(event);
}

int ParameterForm::Item::level()
{
	int l = 0;
	auto p = parent;
	while (p != NULL)
	{
		l++;
		p = p->parent;

	}
	return l;
}

int ParameterForm::Item::isChildOf(int id)
{
	Item * p = parent;
	int l = 0;
	while (p != NULL)
	{
		l++;
		if (p->rec->id == id)
			return l;
		p = p->parent;
	}
	return 0;
}

