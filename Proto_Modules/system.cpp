#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdlib> 
#include <iomanip> 
#include <sstream>

using namespace std;

int main() {

    
    //system("g++ -v| tee new.txt");
    //system("rm new.txt");

    string addr;
    string symbol;
    string cmd;

    
    
    string pathBin;// input Binary-file
    vector<string> pathsBins;// save all Binary-File

    cout << "Input path(s) to Binary: ";
    while (getline(cin, pathBin) && !pathBin.empty()){
        pathsBins.push_back(pathBin);
        cout << "Continue: ";
    }
    cout << "Got it! \n";

    if (pathsBins.empty()){
        cout << "Binary is needed, will abort!";
        abort();
    }
    
    for (int i=0; i <= pathsBins.size() - 1; i++){
        ostringstream tofill;
        tofill << "addr2line -a -f -C -i -s -e " << pathsBins[i] << " 00000000000012e0 | tee -a new.txt";
        cmd = tofill.str();
        cout << cmd << "\n";
        system(cmd.c_str());
        ifstream New ("new.txt");
        getline(New, addr);
        getline(New, symbol);

        if (addr == "0x00000000000012e0" && symbol == "register_tm_clones"){

            cout << pathsBins[i] <<" works!" << endl;
        } else {
            cout << pathsBins[i] << " doesn't work!" << endl;
        }

        New.close();

        system("rm new.txt");
    }
    return 0;

}