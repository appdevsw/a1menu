#ifndef SUTL_H_
#define SUTL_H_

#include <string>
#include <vector>

using std::string;
using std::vector;

// string utilities

class sutl
{
public:
    sutl();
    virtual ~sutl();
    static bool contains(const string&, const string&);
    static bool startsWith(const string&, const string&);
    static bool endsWith(const string&, const string&);
    static string lower(const string&);
    static string trim(const string&);
    static int indexOf(const string&, const string&);
    static string encode(const string&, char escape = '\\');
    static string decode(const string&, char escape = '\\');
    static string replace(const string& sorg, const string& src, const string& dst);
    static vector<string> split(const string&, char);
    static void tokenize(const string& buf, vector<string>& arr, const string& delim = "");
    static string itoa(long long int n);
    static string prepareMarkup(const string&);

    template<typename ... Args>
    static string format(const std::string& format, Args ... args)
    {
        size_t size = snprintf(nullptr, 0, format.c_str(), args ...) + 1;
        char buf[size];
        snprintf(buf, size, format.c_str(), args ...);
        return string(buf);
    }

};

#endif /* SUTL_H_ */
