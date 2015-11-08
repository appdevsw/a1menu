#ifndef TOOLKIT_H
#define TOOLKIT_H
#include <vector>
#include <QString>

class QMutex;


class Toolkit
{
public:
	typedef unsigned long long microsectype;
	Toolkit();
	virtual ~Toolkit();
	int getDirectory(QString dirName, std::vector<QString> * arr,QString ext="",bool withSub=true);
	void removeRecursively(QString path);
	void tokenize(QString buf, std::vector<QString>& arr, QString delim = "");
	int getFileLines(QString fname, std::vector<QString>& vlines);
	bool fexists(QString fname);
	int runCommand(QString command);
	QString getFileText(QString fname);
	QString getFileHash(QString fname);
	QString getHash(const char * buf,int len);
	microsectype GetMicrosecondsTime();

	class Locker
	{
		QMutex * m = NULL;
		bool locked;
	public:
		Locker(QMutex * m);
		~Locker();
		bool isLocked();
	};



};


#endif // TOOLKIT_H
