#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <fstream>

int main (){
    std::string hexa = "15";
    std::string addr = "16";
    int number = 21;
    std::string hexa2 = hexa;
    //unsigned long long integer = std::stoul(hexa, nullptr, 16);
    std::stringstream sstream;
    size_t size;
    //sstream >> size;
    //sstream << std::hex << std::stoul(addr);
    sstream << std::hex << hexa;
    sstream >> size;
    std::cout << "from: " << hexa << "to: " << size  << " done!";// no arithmetic with size here, leads to overflow
    return 0;
}