#include "application.h"
#include "guiitem.h"
#include "desktoprec.h"
#include "applicationloader.h"
#include "categoryloader.h"
#include "guiparametersform.h"
#include "guisearchbox.h"
#include "toolkit.h"
#include "configmap.h"
#include "properties.h"
#include "applet.h"
#include "x11util.h"
#include "sutl.h"
#include <assert.h>
#include <vector>
#include <sstream>
#include <fstream>
#include <gdk/gdkx.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>

using namespace std;

#undef CFG
#undef CFGI
#undef CFGBOOL

const char * Application::A1MENU_VERSION = "2.0.1";

const char * Application::A1MENU_GTK = "a1menu-gtk";
const char * Application::A1MENU_GTK_RUN = "a1menu-gtk.run";
const char * Application::A1MENU_APPLET_NAME = "A1MenuGtk";
const char * Application::A1MENU_APPLET_FACTORY = "A1MenuGtkFactory";
const char * Application::A1MENU_APPLET_SCHEMA = "org.mate.panel.applet.a1menu-gtk";

Application * app = NULL;

Application::Application()
{
}

Application::~Application()
{
    IFDELETE(config);
    IFDELETE(properties);
    IFDELETE(keyListener);
    IFDELETE(loader);
}

void Application::init(int argc, char ** argv)
{
    this->argc = argc;
    this->argv = argv;

    config = new Config();
    properties = new Properties();
    keyListener = new X11KeyListener();

    string md = menuDir();
    dirCache = md + "cache/";
    dirProperties = md + "properties/";
    dirCacheIcons = dirCache + "icons/";
    dirTranslation = md + "translation/";
    loadFile = dirCache + "/loader.txt";
    configFile = md + A1MENU_GTK + ".conf";
    placementFile = md + "/placement.conf";

    res.parYes = "yes";
    mapTranslations[res.parYes] = _("yes");

    res.parNo = "no";
    mapTranslations[res.parNo] = _("no");

    res.locationAuto = "auto";
    mapTranslations[res.locationAuto] = _("auto");
    res.locationManual = "manually by mouse dragging";
    mapTranslations[res.locationManual] = _("manually by mouse dragging");

    res.locationTop = "top";
    mapTranslations[res.locationTop] = _("top");
    res.locationBottom = "bottom";
    mapTranslations[res.locationBottom] = _("bottom");

    res.labDefault = "default";
    mapTranslations[res.labDefault] = _("default");

}

string Application::menuDir()
{
    return Toolkit().homeDir() + "/." + A1MENU_GTK + "/";
}

GuiWindow * Application::currentWindow()
{
    return guiwnd;
}

string Application::CFG(std::string key)
{
    if (!config->mapref().exists(key))
    {
        printf("\nunknown parameter <%s>", key.c_str());
        exit(1);
    }
    auto v = config->mapref()[key];
    return v;
}

int Application::CFGI(std::string key)
{
    string v = CFG(key);
    if (v.empty())
        return 0;
    return atoi(v.c_str());
}

bool Application::CFGBOOL(std::string key)
{
    string v = CFG(key);
    return v == res.parYes;
}

void Application::kbdListenerCallback(const X11KeyData& kd)
{
    auto td = new TimerRec();
    td->action = TOGGLE_SHOW;
    g_idle_add(onTimer, td);

}

void Application::quit()
{
    app->inotifyMonitor.stop();
    app->keyListener->setKey("");
    IFDELETE(guiwnd);
}

void Application::startGui()
{
    reload(false);
    /*
     X11KeyListener::init();
     keyListener->setCallback(*Application::kbdListenerCallback);
     config->load();
     IFDELETE(loader);
     loader = new Loader();
     loader->runInProcess(argc, argv);
     guiwnd = new GuiWindow();
     guiwnd->create();
     keyListener->setHotKeyFromString(CFG("hotkey"));
     app->reloading = false;

     for (auto path : ApplicationLoader().vPaths)
     inotifyMonitor.addPath(path);

     auto td = new TimerRec();
     td->action = MONITOR;
     g_timeout_add(3000, onTimer, td);
     */
}

void Application::reload(bool clearCache)
{
    auto td = new TimerRec();
    td->action = RELOAD;
    td->clearCache = clearCache;
    g_idle_add(onTimer, td);
}

void Application::onPressAppletButton()
{
    auto td = new TimerRec();
    td->action = TOGGLE_SHOW;
    g_idle_add(onTimer, td);
}

void Application::processEvents(int durationMs)
{
    while (gtk_events_pending())
        gtk_main_iteration();

#ifdef GTK3
    Toolkit t;
    auto t1 = t.GetMicrosecondsTime();
    for (;;)
    {
        gtk_main_iteration_do(false);
        while (gtk_events_pending())
        gtk_main_iteration();
        auto t2 = t.GetMicrosecondsTime();
        if (t2 - t1 > (Toolkit::microsectype) durationMs * 1000)
        break;
    }
#endif
}

void Application::loaderThreadProc(TimerRec * td, bool clearCache, bool doShow)
{
    app->loaderMutex.lock();
    td->loader = new Loader();
    td->loader->runInProcess(argc, argv, clearCache);
    td->action = LOADER_FINISH;
    td->clearCache = clearCache;
    td->doShow = doShow;
    //usleep(500 * 1000);
    g_idle_add(onTimer, td);

}

void Application::startLoaderThread(bool clearCache, bool doShow)
{
    printf("\nreloading menu...");
    config->load();
    auto td = new TimerRec();
    td->thr = new thread(&Application::loaderThreadProc, this, td, clearCache, doShow);
}

void Application::finishLoaderThread(TimerRec * td)
{
    //printf("\nloader finish %i", td->serial);
    td->thr->join();
    delete td->thr;

    if (loader)
        delete loader;
    loader = td->loader;

    if (guiwnd == NULL) //fist initialization
    {
        config->load();
        guiwnd = new GuiWindow();
        guiwnd->create();
        app->reloading = false;

        keyListener->setCallback(*Application::kbdListenerCallback);
        keyListener->setKey(CFG("hotkey"));

        for (auto path : ApplicationLoader().vPaths)
            inotifyMonitor.addPath(path);

        auto td = new TimerRec();
        td->action = MONITOR;
        g_timeout_add(3000, onTimer, td);

    } else
    {
        auto block = isBlocked();
        timerTokenMark = getTimerToken();
        reloading = true;
        guiwnd->searchBox->setText(res.labReloading);
        processEvents();
//    if (loader)
//        delete loader;
//    loader = td->loader;
        guiwnd->populate(td->doShow);
        reloading = false;
        setBlocked(block);
    }
    loaderMutex.unlock();
    printf("\nreloading finished.");

}

/*
 void Application::reloadProc(bool clearCache, bool doShow)
 {
 printf("\nreloading menu...\n");
 //usleep(3000*1000);
 timerTokenMark = getTimerToken();
 auto block = isBlocked();
 reloading = true;
 guiwnd->searchBox->setText(res.labReloading);
 processEvents();
 config->load();
 loader = newLoader();
 loader->runInProcess(argc, argv, clearCache);
 guiwnd->populate(doShow);
 reloading = false;
 setBlocked(block);
 printf("\nreloading finished.\n");

 }*/

gboolean Application::onTimer(gpointer data)
{
    TimerRec * td = (TimerRec *) data;

    if (td->action == LOADER_FINISH)
    {
        app->finishLoaderThread(td);
    }

    if (td->action == RELOAD)
    {
        app->startLoaderThread(td->clearCache, true);
    }

    if (app->guiwnd != NULL)
    {
        int show = false;
        bool visible = gtk_widget_get_visible(app->guiwnd->widget());

        if (td->action == TOGGLE_SHOW)
        {
            show = true;
            app->guiwnd->show(!visible);
        }

        if (td->action == MONITOR && app->guiwnd)
        {
            if (!visible && app->inotifyMonitor.check())
            {
                app->startLoaderThread(false, false);
            }
            return TRUE;
        }

        if (show && app->inotifyMonitor.check())
        {
            printf("\n*.desktop directories changed. Reloading required.");
            app->startLoaderThread(false, true);
        }
    }
    delete td;
    return FALSE;
}

void Application::setBlocked(bool enable)
{
    this->blocked = enable;
    gtk_widget_set_sensitive(guiwnd->toolbar->widget(), !enable);
}

bool Application::isBlocked()
{
    return blocked;
}

bool Application::isReloading(string desc)
{
//if (reloading && !desc.empty())
//    printf("\nisReloading: ignore event %s", desc.c_str());
    return reloading;
}

int Application::getTimerToken()
{
    return timerToken++;
}
bool Application::isTimerTokenValid(int token)
{
    return token >= timerTokenMark;
}

void Application::onNewAppletInstance()
{
    if (guiwnd != NULL && loader != NULL)
    {
        string menuIconPath = app->loader->getCachedIconPath(CFG("icon_menu"), app->menuIconSize);
        Applet::setButton(CFG("menu_label"), menuIconPath);

    }
}

void Application::runMenuProcess(GuiItem * it, string command)
{
    assert(guiwnd!=NULL);
    guiwnd->show(false);
    Toolkit toolkit;
    string cmd;
    if (it != NULL)
    {
        auto & rec = it->rec;
        cmd = it->rec.command;
        string home = toolkit.homeDir();
        string QM = "\"";
        cmd = sutl::replace(cmd, "%F", "");
        cmd = sutl::replace(cmd, "%f", "");
        cmd = sutl::replace(cmd, "%U", "");
        cmd = sutl::replace(cmd, "%u", "");
        cmd = sutl::replace(cmd, "%c", QM + rec.loctitle + QM);
        if (rec.iconName != "")
            cmd = sutl::replace(cmd, "%i", "--icon " + QM + rec.iconName + QM);
        else
            cmd = sutl::replace(cmd, "%i", "");

        string fileman = CFG("command_file_manager");
        if (fileman == "")
            fileman = "caja";
        cmd = sutl::replace(cmd, "$open$", fileman);
        cmd = sutl::replace(cmd, "$filemanager$", fileman);

        if (sutl::contains(cmd, "$desktop$"))
        {
            string desktopPath = toolkit.desktopDir();
            cmd = sutl::replace(cmd, "$desktop$", desktopPath);
        }
        cmd = sutl::replace(cmd, "~", home);
        cmd = sutl::replace(cmd, "$home$", home);
        cmd = sutl::replace(cmd, "$HOME", home);
        cmd = sutl::trim(cmd);
    } else
        cmd = command;

    gtk_entry_set_text(guiwnd->searchBox->getEntry(), "");
    if (it != NULL)
        guiwnd->appList->addToRecent(it);

    int cpid1 = fork();
    if (cpid1 == 0)
    {
        int cpid2 = fork();
        if (cpid2 == 0)
        {
            gint argc;
            gchar **argv;
            GError *error = NULL;
            g_shell_parse_argv(cmd.c_str(), &argc, &argv, &error);
            if (error)
            {
                g_error_free(error);
                exit(1);
            }

            //what about the terminating NULL for execvp? better add manually
            char * argvv[argc + 1];
            for (int i = 0; i < argc; i++)
                argvv[i] = argv[i];
            argvv[argc] = NULL;

            // redirect output
            int fd = open("/dev/null", O_WRONLY);
            if (fd > 0)
            {
                dup2(fd, STDERR_FILENO);
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }

            //mate panel doesn't set this, but some applications need it (qbittorrent)
            const char * xdgdesk = "XDG_CURRENT_DESKTOP";
            if (!getenv(xdgdesk))
            {
                setenv(xdgdesk, "MATE", 0);
            }

            setsid(); //detach
            execvp(argvv[0], argvv);
        } else if (cpid2 > 0)
        {
            exit(0);
        }
    } else if (cpid1 > 0)
    {
        int status;
        waitpid(cpid1, &status, 0);
    }

}

void Application::start(bool inApplet)
{

    if (inApplet)
    {
        startGui();
    } else
    {
        gtk_init(&argc, &argv);
        startGui();
        gtk_main();
    }

}

