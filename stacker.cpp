#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <iomanip> 

#include <map>
#include <vector>
#include <sstream>

using namespace std;


/*        
struct Call<'n> {
    func: &'n str,
    addr: usize,
    org_time: u64,
    time: u64,
    /// Time spent in the called subroutines
    child_duration: u64,
}
Call call1;
*/

struct Call {
    string func; //name/symbol
    size_t addr; //start address
    unsigned long long org_time;
    unsigned long long time;
    /// Time spent in the called subroutines
    unsigned long long child_duration;
};

/*
struct Thread<'n> {
    stack: Vec<Call<'n>>,
    switched: u64,
    last_func: usize,
    last_addr: usize,
}
*/

struct Thread{
    vector<Call> stack;
    unsigned long long switched;
    size_t last_func;
    size_t last_addr;
    //Thread() = default;
    void popstack (unsigned long long time) {
        unsigned long long duration = time - stack.back().time;
        stack.pop_back();
        stack.back().child_duration += duration;
    }
};



int main() {
    
    string pathLog;// input Log
    string pathBin;


    string firstline;
    // sort out relevant info
    //    9000: system.cpu: T0 : 0x80000034 @_try_lottery+10. 0 : amoswap_w[l] a6, a7, (a6)  : MemRead :  D=0x0000000000000000 A=0x80040000
    //      |        |      |         |               |                     |                      |                |                 |
    //     tick    place  thread   register       gem5name               instruction            operation          data             address

    string tick;
    string place;
    string thread;
    string regist;
    string gem5name;
    string task;
    string instruction;
    //string operation;
    //string data;
    //string address;
    string rest;
    string oldinst; //instruction of former line

    fstream New; //New file

    //map<string, int> //
    Thread thread_curr;
    Call call;

    cout << "Input path to Log: " ; // same folder "try2-short.txt", different folder "./input/try2-short.txt"
    getline(cin, pathLog);
    //cout << "Input path to Log: ";
    //getline(cin, pathBin);
    ifstream Log (pathLog);
    getline(Log, firstline); //skip first line of Log

    


    New.open("OutputStack.txt", fstream::out);

    while (getline(Log, tick, ':') && stoul(tick) <= 3816500){ // get tick, till first :
        
        unsigned long long longTick = stoul(tick);

        getline(Log, place, ':'); // get place
        getline(Log, thread, ':'); // get thread
        getline(Log, regist, ':'); // register, have to separate name again
        if (regist.find("@") != string::npos){
            gem5name = regist.substr(regist.find("@") + 1);
            regist.resize(regist.find("@"));
        }
        getline(Log, instruction, ':'); // instruction
        //getline(Log, operation, ':'); // operation
        //getline(Log, data, ':'); // data
        //getline(Log, address, ':'); // address
        getline(Log, rest);

        //create cmd addr2line
        ostringstream tofill;
        tofill << "addr2line -C -s -f -a -i  -b elf64-big -e fw_payload23092026.elf " << regist << "| tee address.txt";
        string cmd = tofill.str();
        system(cmd.c_str());
        
        //get info out of address.txt
        ifstream addr2line ("address.txt");
        string fulladdr;
        string symbol;
        getline(addr2line, fulladdr);
        getline(addr2line, symbol); //2nd line has symbol,
        system("rm address.txt");//remove address.txt
        //string to size_t
        //stringstream addrstream(fulladdr);
        //size_t addr;
        //addrstream >> addr; // returns 0
        size_t addr = std::stoul(fulladdr, nullptr, 16);
        //stringstream addrstream;
        //size_t addr;
        //addrstream << std::hex << fulladdr;
        //addrstream >> addr;
        //New << fulladdr<< "|" << std::stoul(fulladdr, nullptr, 16) <<  "|" << addr <<  "|" << addr2 << "\n";

        //stack call on thread
        if (thread_curr.stack.size() == 0){ //beginn stack
            call = (Call){symbol, addr, longTick, longTick, 0};
            thread_curr.stack.push_back(call);
            thread_curr.switched = 0;
            thread_curr.last_addr = addr;
            thread_curr.last_func = addr;
        } else if (!(thread_curr.stack.back().func == symbol))// a different func appears
        {// detect: new func is at beginning?
            ostringstream tocheck;
            //size_t addrcheck=
            tocheck << "addr2line -C -s -f -a -i  -b elf64-big -e fw_payload23092026.elf " << hex << std::stoul(fulladdr, nullptr, 16) - 1 << "| tee address-1.txt";
            string cmdcheck = tocheck.str();
            system(cmdcheck.c_str());
            //New << cmdcheck << "\n";


            ifstream addr2linecheck ("address-1.txt");
            string fulladdrcheck;
            string symbolcheck;
            getline(addr2linecheck, fulladdrcheck);
            getline(addr2linecheck, symbolcheck);
            system("rm address-1.txt");

            if (symbol != symbolcheck){// it's a beginning, a beginning means a jump to new func
                //New << "jump" << "|" << symbol << "|" << symbolcheck << "\n";
                if (symbolcheck == thread_curr.stack.back().func){ //old func (at fulladdr - 1) continues into new func (at fulladdr = addr)
                    New << "continue" << "\n";
                    if (thread_curr.stack.size() == 0){
                    } else {
                    New << tick << "|" << thread_curr.stack[0].func;
                    for (int i = 1; i < thread_curr.stack.size(); i++) {
                        New << ";";
                        New << thread_curr.stack[i].func;
                    }
                    New << "|" << thread_curr.stack.back().time << "|" << thread_curr.stack.back().child_duration;
                    New << "|" << longTick - (thread_curr.stack.back().time + thread_curr.stack.back().child_duration);
                    New << "\n";
                    }
                    thread_curr.popstack(longTick);
                } else { //it was a big jump
                    New << "jump" << "\n";
                }
                    call = (Call){symbol, addr, longTick, longTick, 0};
                    thread_curr.stack.push_back(call);
                    thread_curr.switched = 0;
                    thread_curr.last_addr = addr;
                    thread_curr.last_func = addr; 
                
            } else { //current func returns to former func
                New << "return" << "\n";
                if (thread_curr.stack.size() == 0){
                } else {
                    New << tick << "|" << thread_curr.stack[0].func;
                    for (int i = 1; i < thread_curr.stack.size(); i++) {
                        New << ";";
                        New << thread_curr.stack[i].func;
                    }
                    New << "|" << thread_curr.stack.back().time << "|" << thread_curr.stack.back().child_duration;
                    New << "|" << longTick - (thread_curr.stack.back().time + thread_curr.stack.back().child_duration);
                    New << "\n";
                }
                thread_curr.popstack(longTick);
                thread_curr.switched = 0;
                thread_curr.last_addr = addr;
                thread_curr.last_func = addr; // where old last_func from (?buffer) //this should be the beginning of the former function 
            }
        } else {
            continue; //also skips the print
        }
        
        /*
        //print stack every cycle, when a change in stack happened
        if (thread_curr.stack.size() == 0){
            continue;
        } else {
        New << tick << "|" << thread_curr.stack[0].func;
        for (int i = 1; i < thread_curr.stack.size(); i++) {
            New << ";";
            New << thread_curr.stack[i].func;
        }
        New << "|" << thread_curr.stack.back().time << "|" << thread_curr.stack.back().child_duration;
        New << "|" << longTick - (thread_curr.stack.back().time + thread_curr.stack.back().child_duration);
        New << "\n";
        }
        */

    }
    Log.close();
    New.close();
    
    return 0;
}