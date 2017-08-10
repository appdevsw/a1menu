#include "application.h"
#include "installer.h"
#include "toolkit.h"
#include "sutl.h"
#include <assert.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>

using namespace std;

#define delim string("*")
#define replacefirst(str,from,to)    sutl::replace(delim + str, delim + from, to)
#define replacelast(str,from,to)     sutl::replace(str + delim, from + delim, to)
#define dbg if(1) printf
#define throwstr(s) throw string(s)

#define CONSOLE_RED     "\033[01;31m"
#define CONSOLE_GREEN   "\033[01;32m"
#define CONSOLE_YELLOW  "\033[01;33m"
#define CONSOLE_RESET   "\x1B[0m"

Installer::Installer()
{
    char buf[257];
    assert(getcwd(buf,sizeof(buf)-1)!=NULL);
    currentWorkingDir = buf;
    exePath = (string) "/usr/bin/" + Application::A1MENU_GTK_RUN;
    appVersion = Application::A1MENU_VERSION;
    architecture = "x86_64";
    gtkVersionSym = "gtk2";
#ifdef GTK3
    gtkVersionSym = "gtk3";
#endif
}

Installer::~Installer()
{
}

void Installer::getEntries(std::vector<Entry> & ventries)
{
    Toolkit t;
    string cmd;
    Entry e;

    int trcount = 0;
    if (t.dirExists(currentWorkingDir + "/src/po/") || t.dirExists(currentWorkingDir + "/po/"))
    {
        dbg("\nsearching for translation files (*.po) in the current directory %s", currentWorkingDir.c_str());
        vector<string> vfiles;
        t.getDirectory(currentWorkingDir, &vfiles);

        string dirTmp = (string) "/tmp/" + Application::A1MENU_GTK + "-translations/";

        cmd = "rm -rf " + dirTmp;
        syscmd(cmd);
        if (t.dirExists(dirTmp))
            throwstr(" Remove directory error: " + dirTmp);

        syscmd("mkdir -p " + dirTmp);

        for (auto f : vfiles)
        {
            string fname = f;
            if (sutl::startsWith(fname, currentWorkingDir))
                fname = replacefirst(fname, currentWorkingDir, "./");
            if (sutl::endsWith(fname, ".po"))
            {
                string shortName = t.shortName(fname);
                string lang = sutl::replace(shortName, ".po", "");
                dbg("\n %s %s", shortName.c_str(), lang.c_str());

                string pkgmofile = "/" + lang + "/LC_MESSAGES/" + Application::A1MENU_GTK + ".mo";
                string mofile = dirTmp + shortName + ".mo";

                cmd = "msgfmt --output-file=" + mofile + " " + fname;
                syscmd(cmd);
                if (!t.fileExists(mofile))
                    throwstr(" translation failed.");
                else
                {

                    dbg("\n creating %s", mofile.c_str());

                }

                e = Entry();
                e.srcPath = mofile;
                e.fileName = string(APP_TEXT_DOMAIN) + pkgmofile;
                ventries.push_back(e);
                trcount++;
            }
        }
    }
    if (!trcount)
    {
        printf(CONSOLE_YELLOW);
        printf(
                "\n\n *** WARNING: no translation files found. They should be located in ./src/po/ subdirectory. Are you in the project directory?\n");
        printf(CONSOLE_RESET);
    }

    e = Entry();
    e.content = "[Applet Factory]"
            "\nId=$factory"
            "\nLocation=$exe"
            "\nName=$factory Factory"
            "\nDescription=$factory Factory\n";

    e.content += string("\n[") + Application::A1MENU_APPLET_NAME + "]";
    e.content += "\nName=$A1G"
            "\nDescription=Searchable menu for MATE Desktop"
            "\nIcon=terminal";
    e.fileName = (string) "/usr/share/mate-panel/applets/org.mate.applets." + Application::A1MENU_GTK + ".mate-panel-applet";
    ventries.push_back(e);

    e = Entry();
    e.content = "[D-BUS Service]"
            "\nName=org.mate.panel.applet.$factory"
            "\nExec=$exe";
    e.fileName = (string) "/usr/share/dbus-1/services/org.mate.panel.applet." + Application::A1MENU_APPLET_FACTORY + ".service";
    ventries.push_back(e);

    /*
     e = Entry();
     e.content = "<schemalist gettext-domain=`mate-applets`>";
     e.content += (string) "\n<schema id=`" + Application::A1MENU_APPLET_SCHEMA + "`>"
     "\n<key name=`hello` type=`s`>"
     "\n <default>''</default>"
     "\n <summary>hello</summary>"
     "\n <description>hello</description>"
     "\n</key>"
     "\n</schema>"
     "\n</schemalist>";
     e.fileName = (string) "/usr/share/glib-2.0/schemas/" + Application::A1MENU_APPLET_SCHEMA + ".gschema.xml";
     ventries.push_back(e);
     */

    e = Entry();
    e.fileName = exePath;
    e.content = "";
    e.srcPath = app->argv[0];
    ventries.push_back(e);

}

void Installer::syscmd(string cmd)
{
    dbg("\nexecuting: %s", cmd.c_str());
    system(cmd.c_str());
}

bool Installer::checkUtilExists(string cmd, string errmsg)
{
    Toolkit t;
    string fname = "/tmp/dpkg-rpmbuild-existence-test";

    unlink(fname.c_str());
    assert(!t.fileExists(fname));
    syscmd(cmd + " > " + fname);

    struct stat st;
    stat(fname.c_str(), &st);

    bool r = st.st_size > 0;
    if (!r && !errmsg.empty())
    {
        printf(CONSOLE_YELLOW);
        printf("\n%s\n", errmsg.c_str());
        printf(CONSOLE_RESET);
        throw 0;
    }

    return r;

}

int Installer::createEntryFile(Entry& e)
{
    Toolkit t;
    e.content = sutl::replace(e.content, "`", "\"");
    e.content = sutl::replace(e.content, "$exe", exePath);
    e.content = sutl::replace(e.content, "$version", appVersion);
    e.content = sutl::replace(e.content, "$factory", Application::A1MENU_APPLET_FACTORY);
    e.content = sutl::replace(e.content, "$A1G", Application::A1MENU_GTK);

    auto name = t.shortName(e.fileName);
    auto path = replacelast(e.fileName, name, "");
    e.tmpFileName = tmpRoot + "/" + e.fileName;
    string tmppath = replacelast(e.tmpFileName, name, "");

    syscmd("mkdir -p " + tmppath);

    if (e.srcPath.empty())
    {
        dbg("\ncreating %s", e.tmpFileName.c_str());
        if (t.save(e.tmpFileName, e.content))
            throwstr(" File creation error: " + e.tmpFileName);
    } else
    {
        string cmd = "cp " + e.srcPath + " " + e.tmpFileName;
        syscmd(cmd);
        if (!t.fileExists(e.tmpFileName))
            throwstr(" File creation error: " + e.tmpFileName);
    }
    return 0;
}

int Installer::createDebPackage(string targetDir)
{
    Toolkit t;
    int ret = 0;
    string cmd;

    if (targetDir.empty())
        targetDir = "./";
    targetDir = sutl::replace(targetDir + "/", "//", "/");

    string dirTmp = (string) "/tmp/" + Application::A1MENU_GTK + "-package/";
    string dirTmpPackage = dirTmp + Application::A1MENU_GTK + "-" + appVersion + "-" + gtkVersionSym + "." + architecture;
    string fnameTmpPackage = dirTmpPackage + ".deb";
    string fnameFinalPackage = targetDir + "/" + t.shortName(fnameTmpPackage);
    string fnameMD5sums = dirTmpPackage + "/DEBIAN/md5sums";

    tmpRoot = dirTmpPackage;

    try
    {

        checkUtilExists("dpkg-deb --version", "dpkg-deb is not installed. Cannot create DEB package.");
        checkUtilExists("fakeroot --version", "fakeroot is not installed. Cannot create DEB package.");

        if (t.fileExists(fnameFinalPackage))
        {
            cmd = "rm " + fnameFinalPackage;
            syscmd(cmd);
            if (t.fileExists(fnameFinalPackage))
            {
                throwstr("Cannot remove " + fnameFinalPackage);
            }

        }

        dbg("\npreparing temporary directory...");

        cmd = "rm -rf " + dirTmp;
        syscmd(cmd);
        if (t.dirExists(dirTmp))
            throwstr(" Remove directory error: " + dirTmp);

        vector<Entry> ventries;
        getEntries(ventries);

        struct stat st;
        stat(app->argv[0], &st);
        int pkgsize = st.st_size / 1024;

        auto e = Entry();
        e.content = "Package: $A1G"
                "\nVersion: $version"
                "\nArchitecture: amd64"
                "\nMaintainer: appdevsw <appdevsw@wp.pl>"
                "\nInstalled-Size: $pkgsize"
                "\nSection: admin"
                "\nPriority: optional"
                "\nDescription: Searchable menu for MATE desktop"
                "\n";
        e.content = sutl::replace(e.content, "$pkgsize", sutl::itoa(pkgsize));
        e.fileName = "/DEBIAN/control";
        e.md5 = false;
        ventries.push_back(e);

        string md5sums = "";
        for (auto e : ventries)
        {
            createEntryFile(e);

            if (e.md5)
            {
                string filebuf;
                t.load(e.tmpFileName, filebuf);
                string md5 = t.md5(filebuf);
                string realname = sutl::replace(e.tmpFileName, dirTmpPackage, "");
                realname = sutl::replace(realname, "//", "/");
                md5sums += (md5sums.empty() ? "" : "\n") + md5 + "  " + realname;
            }

        }
        if (t.save(fnameMD5sums, md5sums))
            throwstr("File creation error: " + fnameMD5sums);

        cmd = "fakeroot dpkg-deb --build " + dirTmpPackage;
        dbg("\n");
        syscmd(cmd);

        if (!t.fileExists(fnameTmpPackage))
            throwstr(" Error: package not created: " + fnameTmpPackage);

        cmd = "cp " + fnameTmpPackage + " " + targetDir;
        syscmd(cmd);

        if (!t.fileExists(fnameFinalPackage))
            throwstr(" copy package failed " + fnameFinalPackage);
        else
        {
            printf(CONSOLE_GREEN " \n");
            printf("\nSuccess: package %s created.\n\n", fnameFinalPackage.c_str());
        }

    } catch (string err)
    {
        printf(CONSOLE_RED " \n");
        printf("%s\n", err.c_str());
        ret = -1;
    } catch (int warning)
    {
        ret = -2;
    }
    printf(CONSOLE_RESET);
    return ret;
}

int Installer::createRpmPackage(string dirFinalRpm)
{
    Toolkit t;
    int ret = 0;
    string cmd;

    if (dirFinalRpm.empty())
        dirFinalRpm = "./";

    string dirTmp = (string) "/tmp/" + Application::A1MENU_GTK + "-package/";
    string dirTmpPackage = dirTmp + "makerpm";
    tmpRoot = dirTmpPackage + "/tmproot/";
    string fnameSpec = dirTmp + "/" + Application::A1MENU_GTK + ".spec";
    string fnameTmpRpm = dirTmpPackage + "/RPMS/" + architecture + "/" + Application::A1MENU_GTK + "-" + appVersion + "-" + gtkVersionSym
            + "." + architecture + ".rpm";
    string fnameFinalRpm = dirFinalRpm + "/" + t.shortName(fnameTmpRpm);

    try
    {
        checkUtilExists("rpmbuild --version", "rpmbuild is not installed. Cannot create RPM package.");

        if (t.fileExists(fnameFinalRpm))
        {
            cmd = "rm " + fnameFinalRpm;
            syscmd(cmd);
            if (t.fileExists(fnameFinalRpm))
            {
                throwstr("Cannot remove " + fnameFinalRpm);
            }

        }

        dbg("\npreparing temporary directory...");

        cmd = "rm -rf " + dirTmp;
        syscmd(cmd);
        if (t.dirExists(dirTmp))
            throwstr(" Remove directory error: " + dirTmp);

        vector<Entry> ventries;
        getEntries(ventries);

        string spec = (string) "Name: " + Application::A1MENU_GTK;
        spec = spec + "\nVersion: " + appVersion;
        spec = spec + "\nRelease: " + gtkVersionSym;
        spec = spec + "\nSummary:  Searchable menu for MATE"
                "\nLicense: custom"
                "\nURL: git"
                "\nSource0: git"
                "\n"
                "\n%description"
                "\nSearchable menu for MATE"
                "\n"
                //"\n%changelog\n *"
                "\n";

        spec = spec + "\n\n%files";

        for (auto e : ventries)
        {
            spec = spec + "\n" + e.fileName;
            createEntryFile(e);

        }
        t.save(fnameSpec, spec);
        if (!t.fileExists(fnameSpec))
            throwstr(" File creation error: " + fnameSpec);

        cmd = string() + "rpmbuild -bb" +    //
                " --define \"_topdir " + dirTmpPackage + "\" " +    //
                " --noclean " + //
                " --buildroot=" + tmpRoot //
                + " " + fnameSpec;
        syscmd(cmd);

        cmd = "cp " + fnameTmpRpm + " " + dirFinalRpm;
        syscmd(cmd);
        if (!t.fileExists(fnameFinalRpm))
            throwstr(" Error: package not created.");
        else
        {
            printf(CONSOLE_GREEN " \n");
            printf("\nSuccess: package %s created.\n\n", fnameFinalRpm.c_str());
        }

    } catch (string err)
    {
        printf(CONSOLE_RED " \n");
        printf("%s\n", err.c_str());
        ret = -1;
    } catch (int warning)
    {
        ret = -2;
    }
    printf(CONSOLE_RESET);
    printf("\n");
    return ret;
}
