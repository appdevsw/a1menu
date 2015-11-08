#include "resource.h"
#include "toolkit.h"
#include "ctx.h"
#include <QFileInfo>
#include <QDir>
#include <QStringBuilder>

#include <sstream>
#include <fstream>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <algorithm>
#include <assert.h>
#include <string>

using std::vector;
using std::string;

Resource::Resource()
{
}

int Resource::writeHeader(gzFile fzout, QString orgFileName, QString zipFileName)
{
	int size = 0;
	if (orgFileName != "")
	{
		auto fi = QFileInfo(orgFileName);
		size = fi.size();
	} else
		zipFileName = "EOF";

	QByteArray header = QString().sprintf("%s%c%i%c", QS(zipFileName), sep, size, sep).toLocal8Bit();
	int count = gzwrite(fzout, header.data(), header.length());
	qDebug("writeHeader count <%s> %i %i\n", header.data(), header.length(), count);
	return count == header.length() ? 0 : -1;
}

int Resource::writeContent(gzFile fzout, QString orgFileName)
{
	std::ifstream fin;
	fin.open(QS(orgFileName), ::std::ios::binary);
	if (!fin.is_open())
	{
		perror("zlib writeContent");
		return -1;
	}

	char buf[rdsize];
	int count = 0;
	while (!fin.eof())
	{
		fin.read(buf, sizeof(buf));
		if (fin.gcount() > 0)
			count += gzwrite(fzout, buf, fin.gcount());
	}
	fin.close();
	return count == QFileInfo(orgFileName).size() ? 0 : -1;
}

int Resource::readHeader(gzFile fzin, resfile& rf)
{

	int isep = 0;
	char c;
	rf.header = "";
	for (int i = 0;; i++)
	{
		int count = gzread(fzin, &c, 1);
		//qDebug("read %i <%c>", n, c);
		if (count != 1)
		{
			fprintf(stderr, "gzread: header error\n");
			return -1;
		}
		rf.header.append(1, c);
		if (c == sep)
			isep++;
		if (isep == 2)
			break;
	}

	auto spl = QString::fromStdString(rf.header).split(sep);
	rf.fname = spl[0];
	rf.size = spl[1].toInt();
	rf.eof = QString::fromStdString(rf.header) == eofHeader;
	return 0;
}

int Resource::readContent(gzFile fzin, resfile& rf)
{
	rf.content.reserve(rf.size + 1);
	//qDebug("zlib decompress %s %i", QS(rf.fname), rf.size);
	char bdata[rdsize];
	int pos = 0;
	while (pos < rf.size)
	{
		int lmax = std::min((int) sizeof(bdata), rf.size - pos);
		int count = gzread(fzin, bdata, lmax);
		if (count < 0)
		{
			fprintf(stderr, "gzread: content error\n");
			return -4;
		}
		pos += count;
		//	qDebug("read2 %i %i", n, pos);
		rf.content.append(bdata, count);
	}
	return 0;
}

int Resource::createResources(QString projectPath, QString outputFile)
{
	int retcode = 0;
	gzFile fzout = NULL;
	try
	{
		projectPath.replace("//", "/");

		fzout = gzopen(QS(outputFile), "wb");
		if (fzout == NULL)
		{
			fprintf(stderr, "gzopen: %s: %s\n", QS(outputFile), strerror(errno));
			throw 1;
		}

		Toolkit toolkit;
		vector<QString> v;
		toolkit.getDirectory(projectPath + "/ts", &v);
		toolkit.getDirectory(projectPath + "/css", &v);

		for (auto fname : v)
		{
			qDebug("compressing %s", QS(fname));
			auto fi = QFileInfo(fname);
			QString hname = fname;
			hname.replace(projectPath, "");

			int res = writeHeader(fzout, fname, hname);
			if (res)
			{
				fprintf(stderr, "writeHeader: %s: %s\n", QS(fname), strerror(errno));
				throw 2;
			}

			res = writeContent(fzout, fname);
			if (res)
			{
				fprintf(stderr, "writeContent: %s: %s\n", QS(fname), strerror(errno));
				throw 3;
			}

		}
		writeHeader(fzout, "", "");
	} catch (int e)
	{
		qDebug("createResources exception %i", e);
		retcode = -1;
	}

	if (fzout != NULL)
		gzclose(fzout);

	//test
	//decompress(outputFile, "/tmp/a1menu_restest/");

	return retcode;
}

int Resource::decompress(QString resFile, QString outputPath)
{
	gzFile fzin = NULL;
	std::ofstream fout;
	int retcode = 0;
	try
	{
		fzin = gzopen(QS(resFile), "rb");
		if (fzin == NULL)
		{
			perror("gzopen");
			throw 3;
		}

		for (;;)
		{

			resfile rf;
			int res = readHeader(fzin, rf);
			if (res)
			{
				throw res;
			}

			if (rf.eof)
				break;

			res = readContent(fzin, rf);
			if (res)
			{
				throw res;
			}

			QString outname = outputPath + "/" + rf.fname;
			QDir dir;
			dir.mkpath(QFileInfo(outname).path());
			fout.open(QS(outname), std::ios::out | std::ios::binary);
			if (!fout)
				throw -5;
			const char * buf = rf.content.c_str();
			fout.write(buf, rf.content.length());
			if (fout.bad())
				throw -6;
			fout.close();

		}
	} catch (int e)
	{
		qDebug("decompress exception %i", e);
		retcode = -1;
	}

	if (fzin != NULL)
		gzclose(fzin);
	if (fout)
		fout.close();
	return retcode;
}

int Resource::openFromBuffer(char * bufin, int buflen, std::vector<resfile>& vfiles)
{
	//TIMERINIT();
	int retcode = 0;
	gzFile fzin = NULL;
	FILE * ftmp = NULL;
	int fdtmp = -1;
	try
	{
		ftmp = tmpfile();
		assert(ftmp!=NULL);
		fdtmp = dup(fileno(ftmp));
		int pos = 0;
		while (pos < buflen)
		{
			int lmax = std::min(buflen - pos, wrsize);
			int count = write(fdtmp, bufin + pos, lmax);
			if (count < 0)
			{
				perror("write");
				throw 2;
			}
			pos += count;
			//qDebug("dbg child %i write %i/%i sum %i", getpid(), lbuf, l, pos);
		}
		lseek(fdtmp, 0L, SEEK_SET);
		fzin = gzdopen(fdtmp, "rb");
		if (fzin == NULL)
		{
			perror("gzdopen");
			throw 3;
		}

		for (;;)
		{

			resfile rf;
			int res = readHeader(fzin, rf);
			if (res)
			{
				throw res;
			}
			if (rf.eof)
				break;

			res = readContent(fzin, rf);
			if (res)
			{
				throw res;
			}

			vfiles.push_back(rf);
		}
	} catch (int e)
	{
		qDebug("openFromBuffer exception %i", e);
		retcode = -1;
	}

	if (fzin != NULL)
		gzclose(fzin); //it also closes fdtmp
	else if (fdtmp > 0)
		close(fdtmp);
	if (ftmp != NULL)
		fclose(ftmp);

	//qDebug("Resource::openFromBuffer finished");

	//TIMER("time openFromBuffer", t1, t2);

	return retcode;
}

