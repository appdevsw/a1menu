#include "toolkit.h"
#include "sutl.h"
#include <string>
#include <cstring>
#include <algorithm>
#include <dirent.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <fstream>
#include <unistd.h>
#include <sys/time.h>
#include <assert.h>
#include "md5.h"
//#include <openssl/md5.h>
//#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
//#include <cryptopp/md5.h>

using namespace std;

Toolkit::Toolkit()
{
}

Toolkit::~Toolkit()
{
}

int Toolkit::getDirectory(const string& dirName, vector<string> * arr, const string& ext)
{
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir(dirName.c_str())) != NULL)
    {
        while ((ent = readdir(dir)) != NULL)
        {
            string fname = ent->d_name;
            if (fname != "." && fname != "..")
            {
                string child = dirName;
                if (!sutl::endsWith(child, "/"))
                    child += "/";
                child += fname;
                if (ent->d_type == DT_DIR || (ent->d_type == DT_LNK && dirExists(child)))
                {
                    getDirectory(child, arr);
                } else
                {
                    string su = suffix(child);
                    if (!ext.empty() && ext.find(su) == string::npos)
                        continue;
                    arr->push_back(child);
                }
            }
        }
        closedir(dir);
    } else
    {
        printf("\n");
        perror(dirName.c_str());
        return -1;
    }
    return 0;
}

string Toolkit::shortName(const string& s)
{
    size_t pos = s.find_last_of("/");
    if (pos == string::npos)
        return s;
    return s.substr(pos + 1);
}

string Toolkit::suffix(const string& s)
{
    size_t pos = s.find_last_of(".");
    if (pos == string::npos)
        return "";
    return s.substr(pos + 1);
}

int Toolkit::getFileLines(const string& fname, vector<string>& vlines)
{
    ifstream infile(fname);
    string line;
    while (getline(infile, line))
    {
        vlines.push_back(line);
    }
    infile.close();
    return 0;
}

string Toolkit::homeDir()
{
    struct passwd *pw = getpwuid(getuid());
    assert(pw != NULL && pw->pw_dir != NULL);
    return string(pw->pw_dir);
}

string Toolkit::desktopDir()
{
    Toolkit toolkit;
    string desktopPath = "~/Desktop";
    vector<string> v;
    string home = homeDir();
    getFileLines(home + "/.config/user-dirs.dirs", v);
    for (auto s : v)
    {
        if (sutl::startsWith(s, "XDG_DESKTOP_DIR="))
        {
            s = sutl::replace(s, "XDG_DESKTOP_DIR=", "");
            s = sutl::replace(s, "\"", "");
            desktopPath = s;
            break;
        }
    }
    desktopPath = sutl::replace(desktopPath, "$HOME", home);
    desktopPath = sutl::replace(desktopPath, "~/", home + "/");
    return desktopPath;
}

bool Toolkit::fileExists(const string& fname)
{
    return access(fname.c_str(), 0) == 0;
}

bool Toolkit::dirExists(const string& pathname)
{
    struct stat info;
    return stat(pathname.c_str(), &info) == 0 && (info.st_mode & S_IFDIR);
}

void Toolkit::load(const string& fname, string& buf)
{
    ifstream infile(fname);
    std::getline(infile, buf, char(-1));
    infile.close();

}

int Toolkit::save(const string& fname, const string& buf)
{
    std::ofstream f;
    f.open(fname, std::ios::out | std::ios::binary | std::fstream::app);
    if (!f.is_open())
        return -1;
    f << buf;
    f.close();
    return 0;
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

string Toolkit::md5(const string& inbuf)
{
    /* using CryptoPP
     using namespace CryptoPP::Weak1;
     byte digest[MD5::DIGESTSIZE];
     MD5().CalculateDigest(digest, (unsigned char*) inbuf.c_str(), inbuf.length());
     string md5=std::string((char*) digest, MD5::DIGESTSIZE);
     assert(md5.length()==16);
     std::stringstream ss;
     char buf[8];
     for (int i = 0; i < 16; i++)
     {
     sutl::format(buf, "%02x", (md5.at(i) & 0xFF));
     ss << buf;
     }
     return ss.str();
     */

    /* using OpenSSL
     MD5_CTX c;
     unsigned char digest[16];
     MD5_Init(&c);
     MD5_Update(&c, (unsigned char*) inbuf.c_str(), inbuf.length());
     MD5_Final(digest, &c);
     std::stringstream ss;
     char buf[8];
     for (int i = 0; i < 16; i++)
     {
     sutl::format(buf, "%02x", digest[i]);
     ss << buf;
     }
     return ss.str();
     */

    static bool init = false;
    if (!init)
    {
        init = true;
        assert(md5("") == "d41d8cd98f00b204e9800998ecf8427e");
    }
    return md5::digest(inbuf);

}

string Toolkit::fileMD5(const string& fname)
{
    ifstream infile(fname);
    string buf;
    std::getline(infile, buf, char(-1));
    infile.close();
    return md5(buf);
}

