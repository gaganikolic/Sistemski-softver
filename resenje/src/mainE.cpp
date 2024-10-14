#include <iostream>
#include <stdio.h>
#include <string>
#include "../inc/emulator.hpp"

using namespace std;
 
int main(int argc, char* argv[]) {
    if(argc < 2) {
        cout << "Nije prosledjen ulazni fajl za linker." << endl;
        return -1;
    }  

    Emulator emulator(argv[1]);  
    if (emulator.readInputFile() != 1) {
        cout << "Greska pri citanju ulaznog fajla." << endl;
        return -1;
    }
    emulator.memoryInit();
    emulator.runEmulator();
    emulator.writeRegisters();
    

    return 0;
}
