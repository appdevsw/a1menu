#include <imgconv.h>
#include "mainwindow.h"
#include <QApplication>
#include <QFuture>
#include <QtConcurrentRun>
#include <unistd.h>
#include <signal.h>
#include <malloc.h>
#include "ctx.h"
#include "config.h"
#include "loader.h"
#include "resource.h"
#include <X11/Xlib.h>

ctx::clockmicro_t tbegin;

void *qtApplicationThreadFunc(void * ptdv)
{
	qDebug("Starting Qt thread...");

	ctx::thread_data_t * ptd = &ctx::thread_data;
	QApplication application(ptd->argc, ptd->argv);
	ctx::application = &application;
	MainWindow mainWindow;
	ctx::wnd = &mainWindow;
	ptd->mainQtWindow = &mainWindow;
	if (ptd->inthread)
		ctx::standaloneMode = false;

	mainWindow.init();

#ifdef CONFSYM
	qDebug("configuration %s", CONFSYM);
#endif

	application.setQuitOnLastWindowClosed(false);
	application.exec();
	qDebug("Exit Qt thread...");
	int ret = 0;
	if (ptd->inthread)
		pthread_exit(&ret);
	return NULL;
}

int applet_main(int, char **);
void setMainArguments(int argc, char ** argv, MainWindow * wnd);

//kill -s SIGUSR1 $(ps -A | grep a1menu.run | awk '{print $1}')


void sig_handler(int signo)
{
	printf("received signal %i\n", signo);
	if (ctx::thread_data.mainQtWindow != NULL)
		ctx::thread_data.mainQtWindow->sendShowEvent();
}

void sig_handler_null(int signo)
{
}

int main(int argc, char *argv[])
{
	ctx::thread_data.tstart = tbegin = ctx::clockmicro();
	signal(SIGUSR1, sig_handler_null);
	XInitThreads();
	ctx::thread_data_t * td = &ctx::thread_data;
	ctx::wnd = NULL;
	td->inthread = 1;
	QString arg1 = argc > 1 ? argv[1] : "";
	if (arg1 == ctx::procParRunStandalone)
	{
		td->inthread = 0;
		if (argc > 2 && QString(argv[2]) == ctx::procParHideCtrlButton)
			td->hideCtrlButton = 1;
	}
	td->argc = argc;
	td->argv = argv;

	if (arg1 == ctx::procParImgCache)
	{
		QApplication application(argc, argv);
		if (argc == 4)
		{
			return ImgConv().createCacheIcon(argv[2], atoi(argv[3]));
		}
		printf("\n%s: missing arguments.", QS(arg1));
		return -1;
	}

	if (arg1 == ctx::procParConfig)
	{
		if (argc < 3)
		{
			printf("\n%s: missing arguments.", QS(arg1));
			exit(1);
		}

		return Config::runDialogProcessProc(QString(argv[2]).toInt());
	}

	if (arg1 == ctx::procParLoader)
	{
		Loader().srvMain();
		return 0;
	}

	if (arg1 == ctx::procParResource)
	{
		if (argc != 4)
		{
			qDebug("for %s expected the project path and the output file", argv[1]);
			exit(1);
		}
		Resource().createResources(argv[2], argv[3]);
		return 0;
	}

	ctx::loader = new Loader();
	ctx::loader->initServer();

	signal(SIGUSR1, sig_handler);

	if (td->inthread) //run in MATE panel
	{
		ctx::thread_data.isGtkInitialized = 0;

		pthread_t thr;
		int rc = pthread_create(&thr, NULL, qtApplicationThreadFunc, (void*) td);
		if (rc)
		{
			qDebug("Qt thread initialization error %i", rc);
			exit(1);
		}
		pthread_detach(thr);

		int count = 0;
		while (td->mainQtWindow == NULL)
		{
			if (++count % 50 == 0)
				printf("wait for Qt thread...\n");
			usleep(1000);
		}

		setMainArguments(argc, argv, td->mainQtWindow);
		applet_main(argc, argv);
		//ctx::wnd->quitApp();

	} else //run standalone
	{
		qtApplicationThreadFunc(td);
	}
	printf("Exit main thread.\n");
	return 0;

}

