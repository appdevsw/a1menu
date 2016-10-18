#include "sutl.h"
#include <algorithm>
#include <sstream>
#include <string.h>
#include <ctype.h>
#include <assert.h>

sutl::sutl()
{
}

sutl::~sutl()
{
}

bool sutl::contains(const string& s, const string& x)
{
    return s.find(x) != string::npos;
}

bool sutl::startsWith(const string& s, const string& x)
{
    return s.compare(0, x.length(), x) == 0;
}

bool sutl::endsWith(const string& s, const string& x)
{
    if (s.length() >= x.length())
    {
        return (0 == s.compare(s.length() - x.length(), x.length(), x));
    }
    return false;
}

string sutl::lower(const string& s)
{
    string x = s;
    transform(x.begin(), x.end(), x.begin(), ::tolower);
    return x;
}

string sutl::trim(const string& ss)
{
    auto s = ss;
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
    s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
    return s;
}

int sutl::indexOf(const string& s, const string& x)
{
    auto pos = s.find(x);
    if (pos == string::npos)
        return -1;
    return (int) pos;
}

string sutl::itoa(long long int n)
{
    return sutl::format("%lld", n);
}

std::vector<string> sutl::split(const string& s, char delim)
{
    std::vector<string> v;
    std::stringstream ss;
    ss.str(s);
    string item;
    while (getline(ss, item, delim))
    {
        v.push_back(item);
    }
    return v;
}

string sutl::encode(const string& msg, char escape)
{
    std::stringstream buf;
    for (size_t i = 0; i < msg.length(); i++)
    {
        char cc = msg[i];
        int ci = cc & 0xFF;
        if (ci < 32 || ci > 127 || ci == escape)
            buf << sutl::format("%c%03i", escape, ci);
        else
            buf << cc;
    }
    return buf.str();
}

string sutl::decode(const string& line, char escape)
{
    std::stringstream buf;
    const char * p = line.c_str();
    char c;
    while ((c = *(p++)))
    {
        if (c == escape)
        {
            int code = (*(p) - '0') * 100 + (*(p + 1) - '0') * 10 + (*(p + 2) - '0');
            buf << (char) code;
            p += 3;
        } else
            buf << c;
    }
    return buf.str();
}

string sutl::replace(const string& s, const string& sfrom, const string& sto)
{
    if (sfrom.empty())
        return s;
    size_t istart = 0;
    std::stringstream sn;
    for (;;)
    {
        size_t i = s.find(sfrom, istart);
        if (i == string::npos)
        {
            sn << s.substr(istart);
            break;
        }
        sn << s.substr(istart, i - istart) << sto;
        istart = i + sfrom.length();
    }
    return sn.str();
}

void sutl::tokenize(const string& buf, vector<string>& arr, const string& pdelim)
{
    static string defaultDelim;
    if (defaultDelim.empty() && pdelim.empty())
    {
        for (int i = 1; i <= 32; i++)
            defaultDelim += ((char) i);
    }
    char str[buf.length() + 1];
    strcpy(str, buf.c_str());
    const string& delim = pdelim.empty() ? defaultDelim : pdelim;
    char * pch;
    pch = strtok(str, delim.c_str());
    while (pch != NULL)
    {
        arr.push_back(pch);
        pch = strtok(NULL, delim.c_str());
    }
}

string sutl::prepareMarkup(const string& s)
{
    return replace(s, "&", "&amp;");
}
