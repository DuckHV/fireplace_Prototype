#include <iostream>
#include <fstream>
#include <string>
using namespace std;

int main() {
/*
    ofstream File (, ios::out);
    File.open ("try2-short.txt", std::ifstream::in);
    if (File.is_open()) {
        cout << "Check"; }
    }*/
    ifstream File ("try2-short.txt");

    string s;
 // Read each line from the file
    while (getline(File, s)){
        std::cout << s << std::endl;
   
    }

        
    File.close();

    return 0;
}