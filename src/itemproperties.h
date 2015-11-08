#ifndef ITEMPROPERTIES_H
#define ITEMPROPERTIES_H

#include <QDialog>
#include <QLineEdit>

namespace Ui
{
class ItemProperties;
}

class Item;
class ConfigMap;

class ItemProperties: public QDialog
{
Q_OBJECT

public:
	explicit ItemProperties(Item * item, QWidget *parent = 0);
	~ItemProperties();

	struct prop_name_t
	{
		QString Category = "Category";
		QString IconPath = "IconPath";
		QString CategoryTitle = "CategoryTitle";
		QString InFavorites = "InFavorites";
	};
	static prop_name_t pn;

	int runPropertiesDialog();
	QString propFileName();
	int loadProperties(ConfigMap& tmap);
	QString get(QString prop);
	int set(QString prop, QString val);

private:
	void setGuiFromProperties(ConfigMap & pmap);
	void getPropertiesFromGui(ConfigMap & pmap);
	void save(QString fname,ConfigMap&cmap);
	Ui::ItemProperties *ui;
	Item * item = NULL;
};

#endif // ITEMPROPERTIES_H
