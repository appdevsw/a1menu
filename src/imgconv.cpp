#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <imgconv.h>
#include "toolkit.h"

using std::string;

ImgConv::ImgConv()
{
}

int ImgConv::convert(string sourcePath, string cachePath, int isize)
{
	//No Qt4 code allowed here
	//
	printf("cache icon %s\n", sourcePath.c_str());
	GError * error = NULL;
	GtkImage * img = (GtkImage *) gtk_image_new_from_file(sourcePath.c_str());
	if (img == NULL)
		return -3;
	GdkPixbuf * pixorg = gtk_image_get_pixbuf(img);
	if (pixorg == NULL)
		return -2;
	GdkPixbuf * pix = gdk_pixbuf_scale_simple(pixorg, isize, isize, GDK_INTERP_BILINEAR);
	auto res = gdk_pixbuf_save(pix, cachePath.c_str(), "png", &error, "compression", "0", NULL);
	if (error != NULL)
	{
		printf("\nImgConv error %i %s\n", res, error->message);
		return -4;
	}
	//printf("\n icon created.\n");
	return 0;
}

#include "ctx.h"
#include <QProcess>

QString ImgConv::getCacheName(QString path, int isize)
{
	QString cfname = path;
	cfname.replace("/", "!");
	cfname = PATH(CACHE)+ cfname + ":" + QString::number(isize) + ".png";
	return cfname;
}

int ImgConv::runCacheProcess(QString path, int isize)
{

	qDebug("\ncaching icon: %s:%i", QS(path), isize);
	QProcess process;
	QStringList args;
	args.append(ctx::procParImgCache);
	args.append(path);
	args.append(QString::number(isize));
	process.setProcessChannelMode(QProcess::ForwardedChannels);
	process.start(ctx::thread_data.argv[0], args);
	process.waitForFinished();
	int res = process.exitCode();
	//qDebug("\nprocess finshed with code %i", res);
	return res;
}

int ImgConv::createCacheIcon(QString path, int isize)
{
	QString cacheName = getCacheName(path, isize);
	int cres = convert(path.toStdString(), cacheName.toStdString(), isize);
	return cres ? 0 : -1;
}

QString ImgConv::getPngFromCache(QString ipath, int isize, Mode mode)
{
	Toolkit t;
	QString cacheName = getCacheName(ipath, isize);
	if (t.fexists(cacheName))
		return cacheName;
	if (!t.fexists(ipath))
		return "";
	if (mode == SEPARATE_PROCESS)
		runCacheProcess(ipath, isize);
	else
		convert(ipath.toStdString(), cacheName.toStdString(), isize);
	if (t.fexists(cacheName))
		return cacheName;
	qDebug("Icon cache creation error. Path: %s , Size %i", QS(ipath), isize);
	return "";
}

