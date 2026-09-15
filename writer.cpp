#include <iostream>
#include <fstream>
#include <string>
using namespace std;

int main() {
    ifstream File ("try2-short.txt");
    fstream New;
    string tick;
    string rest;

    New.open("OutputTick.txt", fstream::out); //fstream::out for actually creating file

    while (getline(File, tick, ':')){ // get till first :
    
        New << tick << "\n";
        cout << tick << "\n";
        getline(File, rest); // get/jump till end of line
    }

    File.close();
    New.close();

    return 0;
}