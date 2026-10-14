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

    }
*/

/*
extract (store compatible value in predefined variable, move on)
File >> variable
ws -> skip to next not-whitespace

File >> int Tick >> (ignore place) >> (ignore Thread) >> string Register >> (ignore Task) >> string Instruction >> string Operation >> string D= >> string A=
string A= only for MemRead MemWrite, how to skip
*/

/*
File.getline(destination, size, delimiter) or getline(stream (File), destination, delimiter) 
(read till linebreak/delimiter, discard linebreak/delimiter, put read in String destination)
while(){
    getline(File, Tick, :)
    getline(File, place, :)
    getline(File, Thread, :)
    getline(File, Register, :)
    getline(File, Task, :)
    getline(File, Instruktion, :)
    ignore Operation, D=, A=
    
    getline(File, Operation, :)
    if (Operation == MemRead, MemWrite)
        getline(File, D=, ) //space as delimiter
        getline(File, A=)
    else
        getline(File, D=)
    }
*/
    ifstream File ("try2-short.txt");

    string s;

    while (getline(File, s)){
        std::cout << s << std::endl;
    }

        
    File.close();

    return 0;
}