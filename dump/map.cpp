#include <iostream>
#include <map>
#include <utility>

using namespace std;
 
typedef std::pair<std::string, std::string> src_dst;
typedef map<src_dst, int> Map;
 
 
int main()
{
    std::map<src_dst, int> map =
    {
        { std::make_pair("C++", "C++14"), 2014 },
        { std::make_pair("C++", "C++17"), 2017 },
        { std::make_pair("Java", "Java 7"), 2011 },
        { std::make_pair("Java", "Java 8"), 2014 },
        { std::make_pair("C", "C11"), 2011 }
    };

    // Insert my own pair
    map.insert({{"apple", "pear"}, 66});
 
    for (const auto &entry: map)
    {
        auto key_pair = entry.first;
        std::cout << "{" << key_pair.first << "," << key_pair.second << "}, "
                  << entry.second << std::endl;
    }
 
    return 0;
}