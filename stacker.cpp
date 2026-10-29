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

    void resume (unsigned long long time) {
        unsigned long long duration = time - switched;
        for (int i = 0; i < stack.size(); i++) {
            stack[i].time += duration;
        }
        switched = 0;
    }
};

void print_Stack(fstream& New, Thread thread_curr, unsigned long long longTick){
    if (!thread_curr.stack.empty()){ 
    New << thread_curr.stack[0].func;
    for (int i = 1; i < thread_curr.stack.size(); i++) {
        New << ";";
        New << thread_curr.stack[i].func;
    }
    //New << "|" << longTick << "|" << thread_curr.stack.back().time << "|" << thread_curr.stack.back().child_duration; //debug
    New << " " << longTick - (thread_curr.stack.back().time + thread_curr.stack.back().child_duration) << std::endl;
    }
}


int main() {
    
    string pathLog;// input Log
    string pathBin;// input Binary-file
    vector<string> pathsBins;// save all Binary-File
    int curr_bin;
    
    string fulladdr;// address from addr2line
    string symbol;// name from addr2line
    string file; // file from addr2line

    string firstline;
    // sort out relevant info
    //    9000: system.cpu: T0 : 0x80000034 @_try_lottery+10. 0 : amoswap_w[l] a6, a7, (a6)  : MemRead :  D=0x0000000000000000 A=0x80040000
    //      |        |      |         |               |                     |                      |                |                 |
    //     tick    place  thread   register       gem5name               instruction            operation          data             address

    string tick;
    unsigned long long longTick = 0;
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

    

    unsigned long long const RESERVE = 100;
    // init first thread with id 0
    unsigned long long curr_TID = 0;
    Thread init_Thread;
    map<unsigned long long, Thread> thread_map; //string = value from D=..., sid, a sid per Thread
    thread_map.insert({curr_TID, init_Thread});
    Thread thread_curr = thread_map[curr_TID];
    thread_map.at(curr_TID).stack.reserve(RESERVE);
    thread_curr.stack.reserve(RESERVE);

    unsigned long long const STACK_SIZE = 0x20000;
    
    //Thread thread_curr;
    //thread_curr.stack.reserve(100);

    std::cout << "Input path to Log: " ; // same folder "try2-short.txt", different folder "./input/try2-short.txt"
    getline(cin, pathLog);
    //cout << "Input path to Log: ";
    //getline(cin, pathBin);
    ifstream Log (pathLog);
    getline(Log, firstline); //skip first line of Log

    std::cout << "Input path(s) to Binary: ";
    while (getline(cin, pathBin) && !pathBin.empty()){
        pathsBins.push_back(pathBin);
        std::cout << "Continue: ";
    }
    //std::cout << "Got it! \n";

    if (pathsBins.empty()){
        std::cout << "Binary is needed, will abort!";
        abort();
    }
    


    New.open("OutputStack.folded", fstream::out);
    //New << "please print";
    while (getline(Log, tick, ':')){ // get tick, till first : //limiter: && stoul(tick) <= 4776000 4764000 3816500
        cout << "Tick: " << stoul(tick) << "\n";
        longTick = stoul(tick);

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

        //create cmd addr2line + execute cmd
        for (int i=0; i <= pathsBins.size() - 1; i++){
            ostringstream tofill;
            tofill << "addr2line -a -f -C -i -s -e " << pathsBins[i] << " " << regist << " | tee address.txt";
            string cmd = tofill.str();
            system(cmd.c_str());
            
            //get info out of address.txt
            ifstream addr2line ("address.txt");
            string symbol_buffer;
            getline(addr2line, fulladdr);
            while (getline(addr2line, symbol_buffer)) //use last symbol in addr2line
            {
                symbol = symbol_buffer;
                getline(addr2line, file);
            }
            system("rm address.txt");//remove address.txt

            //check here if name is resolved
            if (symbol == "??"){} // symbol not found, search continues 
            else { // symbol found, remember Binary and go to next step
                curr_bin = i;
                break;
            }
        
        }
        //string to size_t
        //stringstream addrstream(fulladdr);
        //size_t addr;
        //addrstream >> addr; // returns 0
        cout << "Fulladdr: "<< fulladdr << "\n";
        size_t addr = std::stoul(fulladdr, nullptr, 16);
        cout << "addr: " << addr << "\n";
        //stringstream addrstream;
        //size_t addr;
        //addrstream << std::hex << fulladdr;
        //addrstream >> addr;
        //New << fulladdr<< "|" << std::stoul(fulladdr, nullptr, 16) <<  "|" << addr <<  "|" << addr2 << "\n";
        
        //detect first sp assign 
        //bool check = curr_TID == 0 && (instruction.find("c_addi sp,") != string::npos ||instruction.find("c_addi16sp sp,") != string::npos);
        //New << check << std::endl;
        if (curr_TID == 0 && ((instruction.find("c_addi sp,") != string::npos) || (instruction.find("c_addi16sp sp,") != string::npos))){
            //New << instruction.find("c_addi sp,") << " | " << instruction.find("c_addi16sp sp,") << std::endl;
            Thread thread_buffer;
            thread_buffer.stack.reserve(RESERVE);
            thread_buffer = thread_map.at(curr_TID);
            thread_map.erase(curr_TID);
            curr_TID = std::stoul(rest.substr(rest.find("D=") + 2, 18), nullptr, 16); //find in rest, after D= //is that good thread_id?
            thread_map.insert({curr_TID, thread_buffer});
            thread_map.at(curr_TID).stack.reserve(RESERVE);
            thread_curr = thread_map.at(curr_TID);
            thread_curr.stack.reserve(RESERVE);                       
            //print_Stack(New, thread_curr, longTick);
                        
            //New << curr_TID << " | " << std::stoul(rest.substr(rest.find("D=") + 2, 16), nullptr, 16) << " | " << rest.substr(rest.find("D=") + 2, 18) << std::endl;
        }
         


        //detect switch thread
        if ((symbol.find("return_from_user_invoke") != string::npos || symbol.find("return_from_exception") != string::npos) && instruction.find("ld sp, 16(a1)") != string::npos){ // c_ldsp t0, 0(sp)?
            std::cout << "Enter Switch Thread \n";
            unsigned long long thread_id = std::stoul(rest.substr(rest.find("D=") + 2, 18), nullptr, 16);//find in rest, after D=; //is that good thread_id?
            thread_curr.switched = longTick;
            bool empty = true;
            for (auto search : thread_map){
                if(thread_id >= search.first && thread_id < search.first + STACK_SIZE){
                    thread_curr = thread_map.at(search.first);
                    empty =  false;
                    break;
                }
            }
            if (empty){
                thread_map.at(curr_TID) = thread_curr; // stash old thread back into thread_map
                Thread thread_new;
                thread_new.stack.reserve(RESERVE);
                thread_map.insert({thread_id, thread_new}); // create new thread with new ID in thread_map
                thread_map.at(thread_id).stack.reserve(RESERVE);
                thread_curr = thread_map.at(thread_id);
                thread_curr.stack.reserve(RESERVE);
                curr_TID = thread_id; //change curr_TID to new ID
            }
            thread_curr.resume(longTick);
            /*for (auto search : thread_map){
                New << hex << search.first << dec << std::endl;
            }*/ //debug
            //New << "New ID: " << curr_TID << std::endl;
        }
        


        //stack call on thread
        if (thread_curr.stack.size() == 0){ //beginn stack
            Call call = (Call){symbol, addr, longTick, longTick, 0};
            thread_curr.stack.push_back(call);
            thread_curr.last_addr = addr;
            thread_curr.last_func = addr;
        } else if (!(thread_curr.stack.back().func == symbol))// a different func appears
        {// detect: new func is at beginning?
            ostringstream tocheck;
            //size_t addrcheck=
            cout << "Fulladdress-1: " << hex << std::stoul(fulladdr, nullptr, 16) - 1 << dec << "\n";
            tocheck << "addr2line -C -s -f -a -i -e " << pathsBins[curr_bin] << " " << hex << std::stoul(fulladdr, nullptr, 16) - 1 << dec << "| tee address-1.txt";
            string cmdcheck = tocheck.str();
            system(cmdcheck.c_str());
            //New << cmdcheck << "\n";


            ifstream addr2linecheck ("address-1.txt");
            string fulladdrcheck;
            string symbolcheck;
            string symbolcheck_buffer;
            string filecheck;
            getline(addr2linecheck, fulladdrcheck);
            while (getline(addr2linecheck, symbolcheck_buffer))
            {
                symbolcheck = symbolcheck_buffer;
                getline(addr2linecheck,filecheck);
            }
            
            system("rm address-1.txt");

            if (symbol != symbolcheck){// it's a beginning, a beginning means a jump to new func
                //std::cout << "curr_symbol: " << symbol << "\n"; //debug
                //std::cout << "curr_symbolcheck: " << symbolcheck << "\n"; //debug
                //std::cout << "curr_func: " << thread_curr.stack.back().func << "\n"; //debug
                if (symbolcheck == thread_curr.stack.back().func){ //old func (at fulladdr - 1) continues into new func (at fulladdr = addr)       
                    //New << oldinst << "\n"; //debug
                    //New << oldinst.find("jal") << " | " << oldinst.find("c_jalr") << " | "<< string::npos << "\n";  //debug
                    if (thread_curr.stack.size() == 0){ 
                        //std::cout << "stack is empty \n";     
                    } else if (oldinst.find("jal") != string::npos || oldinst.find("c_jalr") != string::npos){//decide with instruction in line, if close jump (call) or continue in new func 
                        //std::cout << "close jump" << "\n; //debug
                    } else {
                        //New << "continue" << "\n"; //debug 
                        print_Stack(New, thread_curr, longTick);
                        thread_curr.popstack(longTick);
                        }
                } else { //it was a big jump
                    //std::cout << "jump" << "\n"; //debug
                }
                Call call = (Call){symbol, addr, longTick, longTick, 0};
                //std::cout << "created call\n"; //debug
                //std::cout << call.func << " " << call.addr << " " << call.time << " " << call.org_time << " " << call.child_duration << "\n"; //debug
                //std::cout << thread_curr.stack.size() << " " << thread_curr.stack.capacity() << " " << thread_curr.stack.max_size()<< "\n";//debug
                thread_curr.stack.push_back(call);
                //std::cout << "pushed call\n";//debug
                thread_curr.last_addr = addr;
                //std::cout << "update func\n";//debug
                thread_curr.last_func = addr; 
                //std::cout << "update last_func\n";//debug
                
            } else { //current func returns to former func 
                //New << "return" << "\n"; //debug
                print_Stack(New, thread_curr, longTick);
                
                thread_curr.popstack(longTick);

                while (!thread_curr.stack.empty()) {// unwind until former func is reached
                if (symbol == thread_curr.stack.back().func){
                    break;
                } else { //delete, check older function in stack
                    //New << "unwrap" << symbol << thread_curr.stack.back().func << "\n";//debug
                    thread_curr.popstack(longTick);
                }
                }

                thread_curr.last_addr = addr;
                thread_curr.last_func = addr; // where old last_func from (?buffer) //this should be the beginning of the former function 
            }
        } else {
            //std::cout << "nothing" << std::endl;
        }

        oldinst = instruction;
        thread_map.at(curr_TID) = thread_curr;
        //std::cout << "looped\n";//debug
        /*
        //print stack every cycle,
        for (auto ID : thread_map){
            thread_curr = ID.second;
            print_Stack(New, thread_curr, longTick);
            //New << hex << " | " << ID.first << dec <<  std::endl;
        }
        thread_curr = thread_map.at(curr_TID);
        */
    }

    //unsigned long long longTick = std::stoul(tick);
    for (auto ID : thread_map){
        thread_curr = ID.second;
        if (ID.first != curr_TID){
            thread_curr.resume(longTick);
        }
        print_Stack(New, thread_curr, longTick);
        
    }

    Log.close();
    New.close();
    std::cout << "\n finished task \n";
    return 0;
}