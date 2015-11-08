#ifndef IMGCONV_H
#define IMGCONV_H

#include <string>
#include <QString>

class ImgConv
{
public:
	enum Mode
	{
		SAME_PROCESS, SEPARATE_PROCESS,
	};
	ImgConv();
	int convert(std::string sourcePath, std::string cachePath, int isize);
	QString getCacheName(QString path, int isize);
	int createCacheIcon(QString path, int size);
	int runCacheProcess(QString path, int size);
	QString getPngFromCache(QString ipath, int isize,Mode mode);
};

#endif // IMGCONV_H
