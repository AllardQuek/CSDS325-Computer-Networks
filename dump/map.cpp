#include <iostream>
#include <map>
#include <unordered_map>
#include <utility>

using namespace std;
 
typedef std::pair<std::string, std::string> src_dst;
typedef map<src_dst, int> Map;

// Create unordered_map
typedef unordered_map<src_dst, int> TrafficMatrix;

 
 
int main()
{
    // std::map<src_dst, int> map =
    // {
    //     { std::make_pair("C++", "C++14"), 2014 },
    //     { std::make_pair("C++", "C++17"), 2017 },
    //     { std::make_pair("Java", "Java 7"), 2011 },
    //     { std::make_pair("Java", "Java 8"), 2014 },
    //     { std::make_pair("C", "C11"), 2011 }
    // };

    std::map<src_dst, int> map;

    // Insert my own pair
    map.insert({{"apple", "pear"}, 66});
    src_dst pair = std::make_pair("apple", "pear");
    map[pair] += 11;

    // Increment for a key that does not already exist
    src_dst new_pair = std::make_pair("x", "y");
    map[new_pair] += 11;
 
    for (const auto &entry: map)
    {
        auto key_pair = entry.first;
        std::cout << "{" << key_pair.first << "," << key_pair.second << "}, "
                  << entry.second << std::endl;
    }
 
    return 0;
}