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
};



int main() {
    
    string pathLog;// input Log
    string pathBin;


    string firstline;
    // sort out relevant info
    //    9000: system.cpu: T0 : 0x80000034 @_try_lottery+10. 0 : amoswap_w[l] a6, a7, (a6)  : MemRead :  D=0x0000000000000000 A=0x80040000
    //      |        |      |                  |                           |                      |                |                 |
    //     tick    place  thread            register                   instruction             operation          data             address

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
    string oldinst; //instruction of former line

    fstream New; //New file

    //map<string, int> //
    Thread thread_curr;

    cout << "Input path to Log: " ; // same folder "try2-short.txt", different folder "./input/try2-short.txt"
    getline(cin, pathLog);
    //cout << "Input path to Log: ";
    //getline(cin, pathBin);
    ifstream Log (pathLog);
    getline(Log, firstline); //skip first line of Log

    


    New.open("OutputStack.txt", fstream::out);

    while (getline(Log, tick, ':') && stoul(tick) <= 1300000){ // get tick, till first :
        
        getline(Log, place, ':'); // get place
        getline(Log, thread, ':'); // get thread
        getline(Log, regist, ':'); // register
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

        //string to size_t
        stringstream addrstream(fulladdr);
        size_t addr;
        addrstream >> addr; // returns 0
        size_t addr2 = std::stoul(fulladdr, nullptr, 16);
        //New << fulladdr<< "|" << std::stoul(fulladdr, nullptr, 16) <<  "|" << addr <<  "|" << addr2 << "\n";
        //stringstream addrstream;
        //size_t addr;
        //addrstream << std::hex << fulladdr;
        //addstream >> addr;

        //stack call on thread
        Call call;
        call = (Call){symbol, addr, 0, 0 ,0};
        if (thread_curr.stack.size() == 0){ //beginn stack
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
            //system("rm address-1.txt");
            if (symbol != symbolcheck){// it's a beginning, a beginning means a jump to new func
                //New << "jump" << "|" << symbol << "|" << symbolcheck << "\n";
                New << "jump" << "\n";
                thread_curr.stack.push_back(call);
                thread_curr.switched = 0;
                thread_curr.last_addr = addr;
                thread_curr.last_func = addr; //this should be the beginning of the former function
                // add case for older func ends, newer func begins
            } else { //current func jumps to former 
                New << "return";
                thread_curr.stack.pop_back();
                thread_curr.switched = 0;
                thread_curr.last_addr = addr;
                thread_curr.last_func = addr;               
            }
        } else {
            continue;
        }
        
        //print stack every cycle 
        if (thread_curr.stack.size() == 0){
            continue;
        } else {
        New << tick << "|" << thread_curr.stack[0].func;
        for (int i = 1; i < thread_curr.stack.size(); i++) {
            New << ";";
            New << thread_curr.stack[i].func;
        }
        New << "\n";
        }

        //remove address.txt
        system("rm address.txt");
        /* 1 thread -> 1 stack

        while (getline(Log, tick, ':')) // till end of Log

        [...]
        void stacker (string addr = 0x80000000, Thread thread) // first find name, then make stack
            
            String cmd = "addr2line -f -a -i  -b elf64-big -e fw_payload.elf %s| tee address.txt" //  not tee -> write only to file, not stdout
            system(String.format(cmd, addr))
            
            ifstream addr2line ("address.txt")
            vector <string> stack

            getline(addr2line, fulladdr)
            getline(addr2line, symbol) //2nd line has symbol,

            if (stack.empty())
            string old_name = symbol
            stack.push_back(symbol)
            else
                if (old_name == symbol)
                    nothing
                else 
                    stack.push_back(;)
                    stack.push_back(symbol)

            system("rm address.txt")

        //
        */

        /*
        if !thread.stack.is_empty() {
        // generate stack
        let stack = match mode {
            crate::Mode::FlameGraph { start, .. } if time >= start => {
                use std::fmt::Write;
                let mut stack: String = format!("{}", tile);
                stack.push(';');
                write!(stack, "{}", tid).unwrap();
                for f in thread.stack.iter() {
                    stack.push(';');
                    stack.push_str(f.func);
                }
                Some(stack)
            },
            _ => None,
        };
        */
    }
    Log.close();
    New.close();
    
    return 0;
}