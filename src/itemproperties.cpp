#include "itemproperties.h"
#include "ui_itemproperties.h"
#include "item.h"
#include "ctx.h"
#include "parameterform.h"
#include "toolkit.h"
#include "configmap.h"

ItemProperties::prop_name_t ItemProperties::pn = ItemProperties::prop_name_t();

ItemProperties::ItemProperties(Item * item, QWidget *parent) :
		QDialog(parent), ui(new Ui::ItemProperties)
{
	ui->setupUi(this);
	this->item = item;
}

ItemProperties::~ItemProperties()
{
	delete ui;
}

void ItemProperties::setGuiFromProperties(ConfigMap& pmap)
{
	this->ui->editCategory->setText(pmap[pn.Category]);
	this->ui->editCategoryTitle->setText(pmap[pn.CategoryTitle]);
	this->ui->editIconPath->setText(pmap[pn.IconPath]);
	this->ui->checkBoxFavorites->setChecked(pmap[pn.InFavorites] == ParameterForm::YES);
}

void ItemProperties::getPropertiesFromGui(ConfigMap& pmap)
{
	pmap[pn.Category] = this->ui->editCategory->text();
	pmap[pn.CategoryTitle] = this->ui->editCategoryTitle->text();
	pmap[pn.IconPath] = this->ui->editIconPath->text();
	pmap[pn.InFavorites] = this->ui->checkBoxFavorites->isChecked() ? ParameterForm::YES : ParameterForm::NO;

}

QString ItemProperties::propFileName()
{
	static QString special = "<>*?+%/\\";
	QString name;
	QString key = item->rec.fname;
	if (key.isEmpty())
		key = item->rec.title;
	for (auto c : key.toAscii())
	{
		if (c <= 32 || c >= 128 || special.contains(c))
			name += QString().sprintf("[%02X]", c & 0xFF);
		else
			name += c;
	}
	return PATH(ITEM_PROPERTIES)+ "/" + (item->isAppItem() ? "app-" : "cat-") + name + ".ipr";
}

int ItemProperties::loadProperties(ConfigMap& tmap)
{
	QString fname = propFileName();
	if (!Toolkit().fexists(fname))
		return -1;
	int res = tmap.load(fname);
	return res;
}

int ItemProperties::runPropertiesDialog()
{
	if (!item->isAppItem())
	{
		ui->editCategory->setVisible(false);
		ui->labCategory->setVisible(false);
		ui->checkBoxFavorites->setVisible(false);
		if (item->load.type & Item::ITEM_TYPE_STATIC)
		{
			ui->editIconPath->setVisible(false);
			ui->labIconPath->setVisible(false);
		}

	} else
	{
		ui->editCategoryTitle->setVisible(false);
		ui->labCategoryTitle->setVisible(false);
	}

	if (item->isPlace())
	{

		ui->editCategory->setVisible(false);
		ui->labCategory->setVisible(false);

		ui->editIconPath->setVisible(false);
		ui->labIconPath->setVisible(false);
	}

	this->setGeometry(QCursor::pos().x(), QCursor::pos().y(), width(), height());

	ConfigMap tmap;
	QString fname = propFileName();
	int res = tmap.load(fname);
	if (res == 0)
	{
		setGuiFromProperties(tmap);
	}

	if (item->isAppItem())
	{
		setWindowTitle(tr("Item preferences") + " [" + item->rec.loctitle + "]");
		ui->editCategory->setFocus();
	} else
	{
		setWindowTitle(tr("Category preferences") + " [" + item->rec.title + "]");
		ui->editIconPath->setFocus();
	}

	setModal(true);
	res = exec();
	if (res)
	{
		ConfigMap tmap;
		getPropertiesFromGui(tmap);
		save(fname, tmap);
	}
	return res;
}

QString ItemProperties::get(QString prop)
{
	ConfigMap itmap;
	ItemProperties::loadProperties(itmap);
	return itmap[prop];
}

int ItemProperties::set(QString prop, QString val)
{
	QString fname = propFileName();
	ConfigMap itmap;
	loadProperties(itmap);
	itmap[prop] = val;
	save(fname, itmap);
	return 0;
}

void ItemProperties::save(QString fname, ConfigMap&cmap)
{
	int notempty = 0;
	for (auto e : cmap.getMap())
	{
		if (e.first == pn.InFavorites.toLower() && e.second == "no")
			continue;
		if (e.second != "")
			notempty++;
	}
	if (notempty)
		cmap.save(fname,true);
	else
		QFile(fname).remove();
}
