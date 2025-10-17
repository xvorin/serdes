#pragma once

#include <string>
#include <vector>

namespace xvorin::serdes {

static inline std::string base64_encode(const std::string& s)
{
    const char* chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string r;
    int val = 0, valb = -6;
    for (unsigned char c : s) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            r.push_back(chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6)
        r.push_back(chars[((val << 8) >> (valb + 8)) & 0x3F]);
    while (r.size() % 4)
        r.push_back('=');
    return r;
}

static inline std::string base64_decode(const std::string& s)
{
    const char* chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string r;
    std::vector<int> t(256, -1);
    for (int i = 0; i < 64; i++)
        t[chars[i]] = i;

    int val = 0, valb = -8;
    for (unsigned char c : s) {
        if (t[c] == -1)
            break;
        val = (val << 6) + t[c];
        valb += 6;
        if (valb >= 0) {
            r.push_back(char((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return r;
}

}