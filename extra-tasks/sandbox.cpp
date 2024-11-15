#include <iostream>
#include <fstream>
#include <string>

int main () {
    std::ifstream is ("text.txt");
    std::string str;
    std::string str2;
    std::cout << is.tellg() << "\n";
    is >> str;
    is >> str2;
    std::cout << str2 << "\n";
}