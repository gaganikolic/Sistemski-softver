#ifndef _EMULATOR_HPP
#define _EMULATOR_HPP

#include <iostream>
#include <fstream>
#include <sys/mman.h> 
#include <vector>

using namespace std;

class Emulator {
private:
  enum Registers {R0, R1, R2, R3, R4, R5, R6, R7, R8, R9, R10, R11, R12, R13, SP, PC};
  enum CS_Regisrets {STATUS, HANDLER, CAUSE};

  struct Section {
    int sectionSize;
    int sectionStartAddress;
    vector<char> sectionContent;

    Section() {}
  };

  void* mem;
  string inputFileName;
  vector<Section> sections;
  unsigned int gpr[16] = {0};
  unsigned int csr[3] = {0};
  bool haltFlag = false;

public:
  Emulator(const string& name) {
    this->inputFileName = name;
  }

  int readInputFile();
  int memoryInit();
  void runEmulator();

  //Instructions handlers
  int aritmeticInstruction(int mod, int regA, int regB, int regC, int disp);
  int logicalInstruction(int mod, int regA, int regB, int regC);
  int callInstruction(int mod, int regA, int regB, int regC, int disp);
  int shiftInstruction(int mod, int regA, int regB, int regC);
  int jumpInstruction(int mod, int regA, int regB, int regC, int disp);
  int storeInstruction(int mod, int regA, int regB, int regC, int disp); 
  int loadInstruction(int mod, int regA, int regB, int regC, int disp);
  int intInstruction();

  void writeRegisters();
};
#endif