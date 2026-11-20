#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <iomanip> 

#include <map>
#include <vector>
#include <sstream>

using namespace std;


struct Call {
    string func; //name/symbol
    size_t addr; //start address
    string bin;
    unsigned long long org_time;
    unsigned long long time;
    /// Time spent in the called subroutines
    unsigned long long child_duration;
};

struct Thread{
    vector<Call> stack;
    unsigned long long switched;
    size_t last_func;
    size_t last_addr;
    //Thread() = default;
    // delete symbol at the back and calculate time difference for new symbol at the back
    void popstack (unsigned long long time) {
        unsigned long long duration = time - stack.back().time;
        stack.pop_back();
        stack.back().child_duration += duration;
    }
    //after pausing + resuming the thread, adjust the time for all symbols in stack with the pause duration 
    void resume (unsigned long long time) {
        unsigned long long duration = time - switched;
        for (int i = 0; i < stack.size(); i++) {
            stack[i].time += duration;
        }
        switched = 0;
    }
};

struct Process{
    map<unsigned long long, Thread> thread_map;
    unsigned long long overwritten_SATP;// this is the former SATP of the process, it can change while in user-mode
};

// to print the stack of a thread
void print_Stack(fstream& New, Thread thread_curr, unsigned long long longTick){
    if (!thread_curr.stack.empty()){
        if (thread_curr.stack[0].func == "??"){
            New << thread_curr.stack[0].func;
        } else {
            New << thread_curr.stack[0].bin + thread_curr.stack[0].func;
        }
        for (int i = 1; i < thread_curr.stack.size(); i++) {
            New << ";";
            if (thread_curr.stack[i].func == "??"){
                New << thread_curr.stack[i].func;
            } else {
                New << thread_curr.stack[i].bin + thread_curr.stack[i].func;  
            }
        }
    New << " " << longTick - (thread_curr.stack.back().time + thread_curr.stack.back().child_duration) << std::endl;
    }
}

//extract here the new SATP from a6
bool satp_from_a6(string instruction, string symbol){
    if (instruction.find("or a6, t0, t5") != string::npos && symbol.find("switchin_context_label") != string::npos){
        return true;
    } else if (instruction.find("or a6, t2, s0") != string::npos && symbol.find("Mem_unit::init_asids()") != string::npos){
        return true;
    } else {
        return false;
    }
}

bool change_satp(string instruction, string symbol){
    if (instruction.find("csrrw zero, satp, a6") != string::npos && symbol.find("switchin_context_label") != string::npos){
        return true;
    } else if (instruction.find("csrrw zero, satp, a6") != string::npos && symbol.find("Mem_unit::init_asids()") != string::npos){
        return true;
    } else {
        return false;
    }
}


int main() {
    
    string pathLog;// input Log
    string pathBin;// input Binary-file
    vector<string> pathsBins;// save all Binary-File
    int curr_bin;// curr_bin = 0 is fiasco binary
    bool context_switched = false; //when a context switch happened -> start new thread after fiasco finished


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
    string rest;
    string oldinst; //instruction of former line

    fstream New; //New file

    //help variables for switching thread/process/context
    unsigned long long new_satp = 0;
    unsigned long long thread_id = 0;

    unsigned long long curr_satp = new_satp;
    Process process;
    process.overwritten_SATP = 0;
    map<unsigned long long, Process> process_map; // contains <SATP, Process>
    process_map.insert({curr_satp, process});
    


    unsigned long long const RESERVE = 100;
    unsigned long long curr_TID = 0;// init first thread with id 0
    Thread init_Thread;
    process_map.at(curr_satp).thread_map.insert({curr_TID, init_Thread});
    process_map.at(curr_satp).thread_map.at(curr_TID).stack.reserve(RESERVE);

    unsigned long long const STACK_SIZE = 0x20000;
    

    std::cout << "Input path to Log: " ;
    getline(cin, pathLog);
    ifstream Log (pathLog);
    getline(Log, firstline); //skip first line of Log

    std::cout << "Input path(s) to Binary (fiasco): "; //pathbin[0] is fiasco
    while (getline(cin, pathBin) && !pathBin.empty()){
        pathsBins.push_back(pathBin);
        std::cout << "Other path to Binary: ";
    }

    if (pathsBins.empty()){
        std::cout << "Binary is needed, will abort!";
        abort();
    }

    vector<string> namesBins;
    for (int i = 0; i <= pathsBins.size() - 1; i++){
        string path = pathsBins.at(i);
        string file = path.substr(path.find_last_of("/\\") + 1);
        size_t lastDot = file.find_last_of(".");
        if (lastDot == string::npos || lastDot == file.size() - 1){
                namesBins.push_back(file + "|");
        } else {
            namesBins.push_back(file.substr(0, lastDot) + "|");
        }
    }

    std::cout << "Got it! \n";
    


    New.open("OutputStack.folded", fstream::out);
    while (getline(Log, tick, ':')){ // get tick, till first : //limiter: && stoul(tick) <= 4776000 4764000 3816500
        //New << "Tick: " << stoul(tick) << std::endl;
        longTick = stoul(tick);
        getline(Log, place, ':'); // get place
        getline(Log, thread, ':'); // get thread
        getline(Log, regist, ':'); // register, have to separate name again
        if (regist.find("@") != string::npos){
            gem5name = regist.substr(regist.find("@") + 1);
            regist.resize(regist.find("@"));
        }
        if (regist.find(".") != string::npos){
            string sub_regist = regist.substr(regist.find(".") + 1);
            regist.resize(regist.find("."));
        }
        getline(Log, instruction, ':'); // instruction
        getline(Log, rest);

        //create cmd addr2line + execute cmd
        for (int i=0; i <= pathsBins.size() - 1; i++){
            ostringstream tofill;
            tofill << "addr2line -a -f -C -i -s -e " << pathsBins[i] << " " << regist << " > address.txt";
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

            //check here if name is resolved
            if (symbol == "??"){
                curr_bin = i;
            } // symbol not found, search continues 
            else { // symbol found, remember Binary and go to next step
                curr_bin = i;
                break;
            }
        
        }

        size_t addr = std::stoul(fulladdr, nullptr, 16);

        //find new SATP
        if (satp_from_a6(instruction, symbol)){// || (instruction.find("or a6, t2, s0") != string::npos && symbol == "Mem_unit::init_asids()")
            new_satp  = std::stoul(rest.substr(rest.find("D=") + 2, 18), nullptr, 16);//find value in a6 to switch to
        }
        
        //process switch, after actually getting new SATP from a6
        if (change_satp(instruction, symbol)){ 
            //New << "ps" << std::endl;
            context_switched = true;
            bool empty = true;
            for (auto SATP : process_map){
                if (SATP.first == new_satp) {
                    empty = false;
                    break;
                }
            }
            if (empty){
                Process new_process;
                process_map.insert({new_satp,new_process});
                process_map.at(new_satp).thread_map.insert({0, init_Thread});
            }
        }
        
        //detect first sp assign 
        if (curr_TID == 0 && ((instruction.find("c_addi sp,") != string::npos) || (instruction.find("c_addi16sp sp,") != string::npos))){
            //New << "spassign" << std::endl;
            Process& curr_process =  process_map.at(curr_satp);
            Thread thread_buffer;
            thread_buffer.stack.reserve(RESERVE);
            thread_buffer = curr_process.thread_map.at(curr_TID);
            curr_process.thread_map.erase(curr_TID);// #abort: corrupted size vs. prev_size
            curr_TID = std::stoul(rest.substr(rest.find("D=") + 2, 18), nullptr, 16); //find in rest, after D= //is that good thread_id?
            curr_process.thread_map.insert({curr_TID, thread_buffer});
            curr_process.thread_map.at(curr_TID).stack.reserve(RESERVE);        
        }        
                
        //detect switch thread, can only happen in fiasco
        if (symbol.find("_skip_save_kernel_sptp78") != string::npos  && instruction.find("c_ldsp sp, 16(sp)") != string::npos){ // c_ldsp t0, 0(sp)? ld sp, 16(a1)return_from_user_invoke|| symbol.find("return_from_exception") != string::npos)
            //New << "ts" << std::endl;
            Process& curr_process =  process_map.at(new_satp);// the process/SATP might have changed beforehand, so the change applies for the next process
            thread_id = std::stoul(rest.substr(rest.find("D=") + 2, 18), nullptr, 16);//find in rest, after D=; //is that good thread_id?
            context_switched = true;
            bool empty = true;
            for (auto search : curr_process.thread_map){
                if(thread_id >= search.first && thread_id < search.first + STACK_SIZE){
                    thread_id = search.first;
                    empty =  false;
                    break;
                }
            }
            if (empty){
                Thread thread_new;
                curr_process.thread_map.insert({thread_id, thread_new}); // create new thread with new ID in thread_map   
            }
        }
                
        //context switch: switch thread on exit fiasco / start application 
        if (context_switched && 0 != curr_bin){ //there was a context switch and leaving fiasco
            process_map.at(curr_satp).thread_map.at(curr_TID).switched = longTick;
            process_map.at(new_satp).thread_map.at(thread_id).stack.reserve(RESERVE);
            process_map.at(new_satp).thread_map.at(thread_id).resume(longTick);
            curr_satp = new_satp; //change curr_SATP to new SATP
            curr_TID = thread_id; //change curr_TID to new TID
            context_switched = false;
        }

        

        Thread& thread_curr = process_map.at(curr_satp).thread_map[curr_TID];

        //stack call on thread
        if (thread_curr.stack.size() == 0){ //beginn stack
            Call call = (Call){symbol, addr, namesBins.at(curr_bin), longTick, longTick, 0};
            thread_curr.stack.push_back(call);
            thread_curr.last_addr = addr;
            thread_curr.last_func = addr;
        } else if (!(thread_curr.stack.back().func == symbol))// a different func appears
        {// detect: new func is at beginning?
            ostringstream tocheck;
            tocheck << "addr2line -C -s -f -a -i -e " << pathsBins[curr_bin] << " " << hex << std::stoul(fulladdr, nullptr, 16) - 1 << dec << " > address-1.txt";
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

            if (symbol != symbolcheck){// it's a beginning, a beginning means a jump to new func
                if (symbolcheck == thread_curr.stack.back().func){ //old func (at fulladdr - 1) continues into new func (at fulladdr = addr)       
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
                Call call = (Call){symbol, addr, namesBins.at(curr_bin), longTick, longTick, 0};
                thread_curr.stack.push_back(call);
                thread_curr.last_addr = addr;
                thread_curr.last_func = addr; 
                
            } else if (symbol == "??"){ // special case when unknown function appears
                Call call = (Call){symbol, addr, namesBins.at(curr_bin), longTick, longTick, 0};
                thread_curr.stack.push_back(call);
                thread_curr.last_addr = addr;
                thread_curr.last_func = addr; 
            } else { //current func returns to former func 
                //New << "return" << "\n"; //debug
                print_Stack(New, thread_curr, longTick);
                
                thread_curr.popstack(longTick);

                while (!thread_curr.stack.empty()) {// unwind until former func in stack is reached or unwind arrived at different binary
                    if (symbol == thread_curr.stack.back().func){
                        break;
                    } else if (namesBins.at(curr_bin) != thread_curr.stack.back().bin && oldinst.find("sret") == string::npos){ //make an exception and push call to stack, when all functions of a binary have been cleared and no return from exception
                        Call call = (Call){symbol, addr, namesBins.at(curr_bin), longTick, longTick, 0};
                        thread_curr.stack.push_back(call);
                        thread_curr.last_addr = addr;
                        thread_curr.last_func = addr;
                        break;
                    } else { //delete, check older function in stack

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
    }
    //cout << "finished reading\n";
    // final print of all threads at end of file
    for (auto SATP : process_map){
        for (auto TID : SATP.second.thread_map){
            if (TID.first != curr_TID && SATP.first != curr_satp){
                    TID.second.resume(longTick);
            }
            print_Stack(New, TID.second, longTick);
        }
    }


    Log.close();
    New.close();
    system("rm address.txt");//remove address.txt
    system("rm address-1.txt");
    std::cout << "\n finished task \n";
    return 0;
}