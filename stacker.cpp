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
    //unsigned long long last_TID;
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
    string new_satp = "0x0000000000000000";
    unsigned long long thread_id = 0;

    //bool process_switched = false;
    string curr_satp = new_satp;
    //string regist_swap_a6 = "0000000000000000"; // the value of satp is stored and changed in register a6
    Process process;
    map<string, Process> process_map;
    process_map.insert({curr_satp, process});
    


    unsigned long long const RESERVE = 100;
    unsigned long long curr_TID = 0;// init first thread with id 0
    Thread init_Thread;
    process_map.at(curr_satp).thread_map.insert({curr_TID, init_Thread});
    process_map.at(curr_satp).thread_map.at(curr_TID).stack.reserve(RESERVE);
    //process_map.at(curr_satp).last_TID = curr_TID;

    unsigned long long const STACK_SIZE = 0x20000;
    

    std::cout << "Input path to Log: " ;
    getline(cin, pathLog);
    ifstream Log (pathLog);
    getline(Log, firstline); //skip first line of Log

    std::cout << "Input path(s) to Binary (fiasco): "; //pathbin[0] is fiasco
    std::cout << "Input path to Binary (fiasco): ";
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
        //New << "Tick: " << stoul(tick) << "\n";
        New << "Tick: " << stoul(tick) << std::endl;
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
            system("rm address.txt");//remove address.txt

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

        
        //swap SATP in process_map, which was changed during application
        if (instruction.find("c_sdsp a6, 128(sp)") != string::npos && symbol == "_save_trap_state0"){
            New << "swapSATP" << std::endl;
            New << "curr_satp: " << curr_satp << "| from_a6: " << rest.substr(rest.find("D=") + 2, 18) << std::endl;New << "curr_TID: "<< hex << curr_TID << "|thread_id: " << thread_id << dec << std::endl;
            New << "curr_satp: "<< hex << curr_satp << "| new_satp: " << new_satp << dec << std::endl;
            New << "Maps: " << std::endl;
            for (auto SATP : process_map){
                for (auto TID : SATP.second.thread_map){
                    New << "SATP: " << SATP.first << "| TID: " << TID.first << std::endl;
                }
            }
            if (rest.substr(rest.find("D=") + 2, 18) != curr_satp){
                Process curr_process =  process_map.at(curr_satp);
                string find_a6 = rest.substr(rest.find("D=") + 2, 18);
                New << "1" << std::endl;
                Process buffer_process = curr_process;
                std::map<string, Process>::iterator it;
                /*
                Process buffer_process;
                New << "2" << std::endl;
                for (auto TID : buffer_process.thread_map){
                    TID.second.stack.reserve(RESERVE);
                }
                
                New << "3" << std::endl;
                buffer_process = curr_process;
                */
                New << "2" << std::endl;
                New << process_map.count(curr_satp) << std::endl;
                it = process_map.find(curr_satp); 
                //New << std::distance(it, process_map.begin()) << std::endl;
                New << "3" << std::endl;//free(): invalid size
                //for (auto TID : buffer_process.thread_map){
                //    New << "curr_satp: "<< process_map.count(curr_satp) << "| TID: " << process_map.at(curr_satp).thread_map.count(TID.first) << std::endl;
                //    process_map.at(curr_satp).thread_map.erase(TID.first);
                //    New << "erased" << std::endl;
                //}
                process_map.insert({find_a6, buffer_process});
                for (auto SATP : process_map){
                for (auto TID : SATP.second.thread_map){
                    New << "SATP: " << SATP.first << "| TID: " << TID.first << std::endl;
                    }
                }
                New << "4" << std::endl;
                process_map.erase(it);
                New << "5" << std::endl;
                New << "6" << std::endl;
                curr_satp = find_a6;
            }
        }
        /*
        //
        if (instruction.find("csrrw zero, satp, a6") != string::npos && symbol == "switchin_context_label"){
            process_switched = true;
        }*/
        //process switch, after actually getting new SATP from a6
        if (instruction.find("c_ldsp a6, 128(sp)") != string::npos && symbol == "_skip_save_kernel_sptp78"){ 
            New << "ps" << std::endl;
            //process_map.at(curr_satp).last_TID=curr_TID;
            context_switched = true;
            bool empty = true;
            new_satp  = rest.substr(rest.find("D=") + 2, 18);//find value in a6 to switch to
            for (auto SATP : process_map){
                if (SATP.first == new_satp) {
                    //curr_satp = new_satp;
                    empty = false;
                    break;
                }
            }
            if (empty){
                Process new_process;
                process_map.insert({new_satp,new_process});
                process_map.at(new_satp).thread_map.insert({0, init_Thread});   
                //curr_satp = new_satp;
            }
        }
        
        //Process& curr_process =  process_map.at(curr_satp);        
                
        //detect switch thread, can only happen in fiasco
        if (symbol.find("_skip_save_kernel_sptp78") != string::npos  && instruction.find("c_ldsp sp, 16(sp)") != string::npos){ // c_ldsp t0, 0(sp)? ld sp, 16(a1)return_from_user_invoke|| symbol.find("return_from_exception") != string::npos)
            New << "ts" << std::endl;
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
            //New << "New ID: " << hex << thread_id << dec << std::endl;
        }
                
        //context switch: switch thread on exit fiasco / start application 
        if (context_switched && 0 != curr_bin){ //there was a context switch and leaving fiasco
            New << "cs" << std::endl;
            New << "curr_TID: "<< hex << curr_TID << "|thread_id: " << thread_id << dec << std::endl;
            New << "curr_satp: "<< hex << curr_satp << "|new_satp: " << new_satp << dec << std::endl;
            New << "Maps: " << std::endl;
            for (auto SATP : process_map){
                for (auto TID : SATP.second.thread_map){
                    New << "SATP: " << SATP.first << "| TID: " << TID.first << std::endl;
                }
            }
            process_map.at(curr_satp).thread_map.at(curr_TID).switched = longTick;
            process_map.at(new_satp).thread_map.at(thread_id).stack.reserve(RESERVE);
            process_map.at(new_satp).thread_map.at(thread_id).resume(longTick);
            curr_satp = new_satp; //change curr_SATP to new SATP
            curr_TID = thread_id; //change curr_TID to new TID
            context_switched = false;
        }

        //detect first sp assign 
        if (curr_TID == 0 && ((instruction.find("c_addi sp,") != string::npos) || (instruction.find("c_addi16sp sp,") != string::npos))){
            New << "spassign" << std::endl;
            Process& curr_process =  process_map.at(curr_satp);
            Thread thread_buffer;
            thread_buffer.stack.reserve(RESERVE);
            thread_buffer = curr_process.thread_map.at(curr_TID);
            curr_process.thread_map.erase(curr_TID);// #abort: corrupted size vs. prev_size
            curr_TID = std::stoul(rest.substr(rest.find("D=") + 2, 18), nullptr, 16); //find in rest, after D= //is that good thread_id?
            curr_process.thread_map.insert({curr_TID, thread_buffer});
            curr_process.thread_map.at(curr_TID).stack.reserve(RESERVE);        
        }

        Thread& thread_curr = process_map.at(curr_satp).thread_map[curr_TID];
        
        New << "curr_satp: " << curr_satp << " | curr_TID: " << curr_TID << std::endl;

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
            
            system("rm address-1.txt");

            if (symbol != symbolcheck){// it's a beginning, a beginning means a jump to new func
                //New << "thread_curr: capacity: " << thread_curr.stack.capacity() << "|size: " << thread_curr.stack.size() << std::endl;
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
        New << "looped" << std::endl;
        /*
        New << "current TID: " << hex << curr_TID << dec << "\n";
        cout << "current TID: " << hex << curr_TID << dec << "\n";
        for (auto SATP : process_map){
            for (auto TID : SATP.second.thread_map){
                if (TID.first != curr_TID){
                    TID.second.resume(longTick);
                }
                New << SATP.first << ": " << hex << TID.first << dec << ":\n";
                print_Stack(New, TID.second, longTick);
            }
        }
        */
    }
    //cout << "finished reading\n";
    // final print of all threads at end of file
    for (auto SATP : process_map){
        for (auto TID : SATP.second.thread_map){
            if (TID.first != curr_TID && SATP.first != curr_satp){
                    TID.second.resume(longTick);
            }
            //New << SATP.first << ": " << hex << TID.first << dec << ":\n";
            print_Stack(New, TID.second, longTick);
        }
    }


    Log.close();
    New.close();
    std::cout << "\n finished task \n";
    return 0;
}