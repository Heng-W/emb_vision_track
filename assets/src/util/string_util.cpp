
#include "string_util.h"
#include <algorithm>

using namespace std;

namespace util
{

// 分割字符串
vector<string> splitString(const string& str, const string& delim)
{
    vector<string> res;
    auto start = str.cbegin();
    while (start < str.cend())
    {
        auto it = std::search(start, str.cend(), delim.cbegin(), delim.cend());
        res.emplace_back(start, it);
        start = it + delim.size();
    }
    return res;
}

} // namespace util