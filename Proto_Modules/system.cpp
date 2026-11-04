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
    string file;
    int curr_bin;
    
    
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
        tofill << "addr2line -a -f -C -i -s -e " << pathsBins[i] << " 0x8050037e > new.txt";
        cmd = tofill.str();
        cout << cmd << "\n";
        system(cmd.c_str());
        ifstream New ("new.txt"); 
        string symbol_buffer;
        getline(New, addr);
        while (getline(New, symbol_buffer)) //use last symbol in addr2line
            {
                symbol = symbol_buffer;
                getline(New, file);
            }

        //check here if name is resolved
        if (symbol == "??"){
            curr_bin = i;
            cout << "none" << std::endl;
        } // symbol not found, search continues 
        else { // symbol found, remember Binary and go to next step
            curr_bin = i;
            cout << symbol << std::endl;
            break;
        }

        /*
        if (addr == "0x00000000000012e0" && symbol == "register_tm_clones"){

            cout << pathsBins[i] <<" works!" << endl;
        } else {
            cout << pathsBins[i] << " doesn't work!" << endl;
        }
        */

        New.close();

        system("rm new.txt");
    }
    cout << "final symbol: " << symbol << std::endl;
    return 0;

}