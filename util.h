#ifndef UTIL_H_INCLUDED
#define UTIL_H_INCLUDED

#include <string>
#include <sstream>

template<typename T>
std::string hexstring(T val, int w = 2 * sizeof(T))
{
    std::string res(w, '\0');
    for (int pos = w - 1; pos >= 0; pos--, val >>= 4) {
        res[pos] = "0123456789abcdef"[val & 0xf];
    }
    return res;
}

template<typename T>
std::string with_width(const T& t, size_t width)
{
    std::ostringstream oss;
    oss << t;
    auto res = oss.str();
    size_t w = 0;
    for (const auto ch : res) {
        if (ch == '\t')
            w += 8 - (w % 8);
        else
            ++w;
    }
    if (w < width)
        res.insert(res.end(), width - w, ' ');
    return res;
}

#endif
