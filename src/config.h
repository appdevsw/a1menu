#ifndef CONFIG_H
#define CONFIG_H

#include <QDialog>
#include <QFormLayout>
#include <QListWidget>
#include "configmap.h"
#include "parameterform.h"
#include <QLineEdit>
#include <QComboBox>
#include <vector>
#include <QTranslator>
#include <QCoreApplication>
#include <QFileSystemWatcher>

namespace Ui
{
class Config;
}

struct ConfigItem
{
	int id;
	int parentid;
	int type = 0;
	bool mandatory = false;
	QString name;
	QString value;
	QString labelorg;
	QString label;
	std::vector<QString> validitems;
	QString rangeMin = 0;
	QString rangeMax = 0;
	bool reloadreq = false;
	bool isCategory = false;
	bool hotkey = false;
	int ord = 100;
};

class Config
{
Q_DECLARE_TR_FUNCTIONS(Config)
public:
	explicit Config();
	virtual ~Config();
	void setDefaultConfig(ConfigMap& cmap);

	static int runDialogProcess();
	static int runDialogProcessProc(int iofd);

	static void inheritIncludes(ConfigMap& cfgorg, ConfigMap& cfg);
	//bool parameterFormCallback(ParameterForm * pf,int buttonCode);
	static QString getDesktopPath();
	static void initTranslation();
	static QString getVTrans(QString txt, int reverse = 0);
	static QString getStyleFile();
	static void installDefaultFiles();
	vector<ConfigItem *> * items();

private:

	static std::map<QString, QString> vtrans;
	vector<ConfigItem *> vitems;

	int newItem(int parentid, QString name, int type = 0, QString value = "", QString label = "", QString lov = "", //
			bool mandatory = false, bool reload = false);

};

#endif // CONFIG_H
