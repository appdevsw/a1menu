#include <gtk/gtk.h>
#include <stdio.h>
#include <assert.h>
#include <vector>

#include "application.h"
#include "guiparametersform.h"
#include "x11util.h"
#include "sutl.h"
#include "installer.h"
#include "applet.h"

using namespace std;

void sig_handler(int signo)
{
    if (signo == app->SIG_RELOAD)
    {
        app->reload(false);
    }
    if (signo == app->SIG_CLEAR_CACHE)
    {
        app->reload(true);
    }
}

int main(int argc, char *argv[])
{
    setvbuf(stdout, NULL, _IONBF, 0); //printf & \n issue
    setvbuf(stderr, NULL, _IONBF, 0);
    XInitThreads();

    string arg, arg2;
    if (argc > 1)
        arg = argv[1];
    if (argc > 2)
        arg2 = argv[2];

    // --- init translations
    string domain = Application::A1MENU_GTK;
    setlocale(LC_ALL, "");
    bindtextdomain(domain.c_str(), APP_TEXT_DOMAIN);
    textdomain(domain.c_str());

    if (argc == 0 || arg == "--run" || arg == "--applet")
        printf("\ntranslation domain: %s, location: %s", textdomain(NULL), bindtextdomain(domain.c_str(), NULL));
    // -------------------

    app = new Application();
    app->init(argc, argv);

    if (arg == string("--makedeb"))
    {
        Installer().createDebPackage(arg2);
        exit(0);
    }
    if (arg == string("--makerpm"))
    {
        Installer().createRpmPackage(arg2);
        exit(0);
    }

    if (arg == string("--load"))
    {
        gtk_init(&argc, &argv);
        (new Loader())->loadProc();
        exit(0);
    }

    if (arg == string("--pform"))
    {
        if (argc != 3)
        {
            printf("\n --pform : missing PPID");
            exit(1);
        }
        int ppid = atoi(argv[2]);
        gtk_init(&argc, &argv);
        GuiParametersForm pf;
        int result = pf.create(ppid);
        exit(result);
    }

    if (arg == string("--run"))
    {
        signal(app->SIG_RELOAD, sig_handler);
        signal(app->SIG_CLEAR_CACHE, sig_handler);
        app->start();
        printf("\nExit");
        exit(0);
    }

    if (arg == string("--applet") || arg.empty())
    {
        signal(app->SIG_RELOAD, sig_handler);
        signal(app->SIG_CLEAR_CACHE, sig_handler);
        Applet::appletMain(argc, argv);
    }

    return 0;
}
