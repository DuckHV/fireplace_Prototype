#include <iostream>
#include <fstream>
#include <string>
using namespace std;

int main() {
    // input Log
    string path;
        
    string tick;
    string place;
    string thread;
    string regist;
    string task;
    string instruction;
    //string operation;
    //string data;
    //string address;
    string rest;

    fstream New;

    cout << "Input path to Log: "; // same folder "try2-short.txt", different folder "./input/try2-short.txt"
    getline(cin, path);
    ifstream Log (path);

    
    // sort out relevant info
    //    9000: system.cpu: T0 : 0x80000034 @_try_lottery+10. 0 : amoswap_w[l] a6, a7, (a6)  : MemRead :  D=0x0000000000000000 A=0x80040000
    //      |        |      |                  |                           |                      |                |                 |
    //     tick    place  thread            register                   instruction             operation          data             address


    New.open("OutputJump.txt", fstream::out);

    while (getline(Log, tick, ':')){ // get tick, till first :
        
        getline(Log, place, ':'); // get place
        getline(Log, thread, ':'); // get thread
        getline(Log, regist, ':'); // register
        getline(Log, instruction, ':'); // instruction
        //getline(Log, operation, ':'); // operation
        //getline(Log, data, ':'); // data
        //getline(Log, address, ':'); // address
        getline(Log, rest);

        //New << instruction << "\n";

        size_t jal = instruction.find("jal"); 
        size_t c_jr = instruction.find("c_jr");
        //New << tick << "|" << jal << "|" << c_jr << "\n";
        
        // sort out relevant info
        if ( jal != string::npos || c_jr != string::npos) { // string::npos is true
            //cout << "1";
            New << tick << "|" << instruction << "\n";
        }
    }

    New << "End";

    Log.close();
    New.close();

    return 0;

}