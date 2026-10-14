#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib> 

using namespace std;

int main() {

    
    //system("g++ -v| tee new.txt");
    //system("rm new.txt");

    string addr;
    string symbol;

    system("addr2line -a -f -C -i -s -e writer.exe 00000000000012e0 | tee -a new.txt");
    ifstream New ("new.txt");
    getline(New, addr);
    getline(New, symbol);

    if (addr == "0x00000000000012e0" && symbol == "register_tm_clones"){

        cout << "it works" << endl;
    } else {
        cout << "it doesn't work" << endl;
    }

    New.close();

    system("rm new.txt");

    return 0;

}