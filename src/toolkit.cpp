#include "toolkit.h"
#include "ctx.h"
#include <string>
#include <cstring>
#include <algorithm>
#include <dirent.h>
#include <QWidget>
#include <QIODevice>
#include <QFile>
#include <QTextStream>
#include <QDirIterator>
#include <QDir>
#include <QFileInfoList>
#include <QFileInfo>
#include <QProcess>
#include <unistd.h>
#include <QCryptographicHash>
#include <QDir>
#include <QMutex>
#include <sys/time.h>
#include <ctype.h>
#include <assert.h>
#include <set>

using namespace std;
using namespace ctx;

Toolkit::Toolkit()
{
}

Toolkit::~Toolkit()
{
}

void Toolkit::tokenize(QString buf, vector<QString>& arr, QString delim)
{
	QString word;
	if (delim == "")
		for (int i = 0; i <= 32; i++)
			delim.append((char) i);
	buf += delim.left(1);
	for (int i = 0; i < buf.length(); i++)
	{
		QString c = buf.mid(i, 1);
		if (delim.contains(c))
		{
			if (word.length() > 0)
			{
				//qDebug("\nToken %s", word.toStdString().c_str());
				arr.push_back(word);
			}
			word = "";
		} else
		{
			word += c;
		}
	}
}

bool Toolkit::fexists(QString fname)
{
	return access(QS(fname), 0) == 0;

}

int Toolkit::getDirectory(QString dirName, vector<QString> * arr, QString ext, bool withSub)
{

	DIR *dir = opendir(QS(dirName));
	if (dir == NULL)
		return -1;

	struct dirent *ent;
	while ((ent = readdir(dir)) != NULL)
	{
		QString fname = ent->d_name;
		if (fname != "." && fname != "..")
		{
			QString childPath = dirName + "/" + fname;
			childPath.replace("//", "/");
			if (ent->d_type == DT_DIR)
			{
				if (withSub)
				{
					getDirectory(childPath, arr, ext, withSub);
					//if (res)
					//	return res;
				}
			} else
			{
				if (ext != "" && !childPath.endsWith(ext))
					continue;
				arr->push_back(childPath);
			}
		}
	}
	closedir(dir);
	return 0;

}

void Toolkit::removeRecursively(QString path)
{
	vector<QString> vfiles;
	getDirectory(path, &vfiles);
	map<QString, QString> vdir;
	//QDir odir;

	//QFile(PATH(UNZIP)+"/resource.tar.gz").remove();

	for (auto fname : vfiles)
	{
		assert(fname.contains("a1menu/"));
		/*
		 auto res = QFile(fname).remove();
		 */

		auto res = unlink(QS(fname));
		if (res)
			qDebug("remove file error %i: %s", res, QS(fname));
		//system(QS("touch "+fname));

		QString path = QFileInfo(fname).path();
		vdir[QString().sprintf("%6i %s", 9999 - path.length(), QS(path))] = path;
	}
	for (auto e : vdir)
	{
		QString fdir = e.second;
		assert(fdir.contains("a1menu/"));
		//auto res = odir.rmdir(fdir);

		auto res = rmdir(QS(fdir));
		if (res)
			qDebug("remove directory error %i: %s", res, QS(fdir));
	}
}

int Toolkit::getFileLines(QString fname, std::vector<QString>& vlines)
{
	FILE *f = fopen(QS(fname), "r");
	if (f == NULL)
	{
		//perror(QS(fname));
		return -1;
	}
	QTextStream in(f, QIODevice::ReadOnly);
	in.setCodec("UTF-8");
	while (!in.atEnd())
	{
		QString l = in.readLine();
		vlines.push_back(l);
		//qDebug("line <%s>", l.toStdString().c_str());
	}
	fclose(f);
	return 0;
}

int Toolkit::runCommand(QString command)
{
	bool res = QProcess::startDetached(command);
	if (!res)
		return -1;
	return 0;
}

QString Toolkit::getFileText(QString fname)
{
	FILE *f = fopen(QS(fname), "r");
	if (f == NULL)
	{
		//perror(QS(fname));
		return "";
	}
	QTextStream in(f, QIODevice::ReadOnly);
	in.setCodec("UTF-8");
	QString buf = in.readAll();
	fclose(f);
	return buf;
}

QString Toolkit::getFileHash(QString fname)
{
	QCryptographicHash crypto(QCryptographicHash::Sha1);
	QFile file(fname);
	file.open(QFile::ReadOnly);
	while (!file.atEnd())
	{
		crypto.addData(file.read(8192));
	}
	file.close();
	QByteArray hash = crypto.result();
	QString h;
	for (auto c : hash)
		h.sprintf("%s%02X", QS(h), c & 0xFF);
	//qDebug("hash %s %s",QS(h),QS(fname));
	return h;
}

QString Toolkit::getHash(const char * buf, int len)
{
	QCryptographicHash crypto(QCryptographicHash::Sha1);
	crypto.addData(QByteArray(buf, len));
	QByteArray hash = crypto.result();
	QString h;
	for (auto c : hash)
		h.sprintf("%s%02X", QS(h), c & 0xFF);
	return h;

}

Toolkit::microsectype Toolkit::GetMicrosecondsTime()
{
	struct timeval tv;
	if (gettimeofday(&tv, nullptr) != 0)
		return 0;
	microsectype lsec = (microsectype) tv.tv_sec;
	lsec *= 1000000ULL;
	microsectype usec = (microsectype) tv.tv_usec;
	return lsec + usec;
}

Toolkit::Locker::Locker(QMutex * m)
{
	this->m = m;
	locked = m->tryLock();
}
Toolkit::Locker::~Locker()
{
	if (locked)
		m->unlock();
}

bool Toolkit::Locker::isLocked()
{
	return locked;
}

