#ifndef RESOURCE_H
#define RESOURCE_H

#include <QString>
#include <vector>
#include <zlib.h>


class Resource
{
public:

	struct resfile
	{
		std::string header;
		QString fname;
		int size;
		std::string content;
		bool eof=false;
	};


	Resource();
	int createResources(QString projectPath, QString outputFile);
	int decompress(QString resFile, QString outputPath);
	int openFromBuffer(char * buf, int len, std::vector<resfile>& vfiles);

private:
	char sep = '*';
	QString eofHeader = QString("EOF") + sep + "0" + sep;
	int rdsize=1024*8;
	int wrsize=1024*8;

	int writeHeader(gzFile fzout,QString orgFileName,QString zipFileName);
	int writeContent(gzFile fzout,QString orgFileName);

	int readHeader(gzFile fzin,resfile& rf);
	int readContent(gzFile fzin,resfile& rf);

};

#endif // RESOURCE_H
