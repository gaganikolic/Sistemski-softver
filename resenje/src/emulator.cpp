#include "../inc/emulator.hpp"
#include <iostream>
#include <fstream>
#include <sys/mman.h> 
#include <iomanip>
#include <vector>

int Emulator::readInputFile() {
  std::ifstream file(inputFileName, std::ios::in | std::ios::binary);
  if (!file) {
    cout << "Nije moguce otvoriti fajl: " << inputFileName << endl;
    return -1;
  }

  int sectionsNum;
  file.read(reinterpret_cast<char*>(&sectionsNum), sizeof(sectionsNum));
  for(int i = 0; i < sectionsNum; i++) {
    int address;
    int sectionSize;
    
    file.read(reinterpret_cast<char*>(&address), sizeof(address));
    file.read(reinterpret_cast<char*>(&sectionSize), sizeof(sectionSize));

    vector<char> sectionContent(sectionSize);
    file.read(sectionContent.data(), sectionSize);

    Section readSection;
    readSection.sectionStartAddress = address;
    readSection.sectionSize = sectionSize;
    readSection.sectionContent = sectionContent;
    sections.push_back(readSection);
  }
  
  return 1;
}

int Emulator::memoryInit() {
  unsigned long size = (1UL << 32);
  mem = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  if(mem == MAP_FAILED) {
    cout << "Neuspesna inicijalizacija memorije!" << endl;
    return false;
  } 
  
  for(auto& sec: sections) {
    unsigned int sectionStartAddress = (unsigned int) sec.sectionStartAddress;
    unsigned int sectionSize = (unsigned int) sec.sectionSize;
    for(int i = 0; i < sectionSize; i++) {
      char* address = (char*)(mem) + (sectionStartAddress + i);
      *address = sec.sectionContent[i];
    }
  }
  return 1;
}

int Emulator::aritmeticInstruction(int mod, int regA, int regB, int regC, int disp) {
  if(mod == 0) {
    gpr[regA] = gpr[regB] + gpr[regC];
  } else if( mod == 1) {
    gpr[regA] = gpr[regB] - gpr[regC];
  } else if(mod == 2) {
    gpr[regA] = gpr[regB] * gpr[regC];
  } else if(mod == 3) {
    gpr[regA] = gpr[regB] / gpr[regC];
  } else {
    cout << "Greska prilikom obrade aritmeticke instrukcije" << endl;
    return -1;
  }
  return 1;
}
int Emulator::logicalInstruction(int mod, int regA, int regB, int regC) {
  if(mod == 0) {
    gpr[regA] = ~gpr[regB];
  } else if(mod == 1) {
    gpr[regA] = gpr[regB] & gpr[regC];
  }else if(mod == 2) {
    gpr[regA] = gpr[regB] | gpr[regC];
  } else if(mod == 3) {
    gpr[regA] = gpr[regB] ^ gpr[regC];
  } else {
    cout << "Greska prilikom obrade logicke instrukcije" << endl;
    return -1;
  }
  return 1;
}
int Emulator::callInstruction(int mod, int regA, int regB, int regC, int disp) {
  unsigned int pom;
  uint32_t* addr;
  uint32_t jumpAddress;
  if(mod == 0) {
    // pc<=gpr[A]+gpr[B]+D
    gpr[SP] = gpr[SP] - 4;
    uint32_t* address = (uint32_t*)((char*)(mem) + gpr[SP]);
    *address = gpr[PC];
    gpr[PC] = gpr[regA] + gpr[regB] + disp;
  } else if(mod == 1) {
    // pc<=mem32[gpr[A]+gpr[B]+D]
    gpr[SP] = gpr[SP] - 4;
    uint32_t* address = (uint32_t*)((char*)(mem) + gpr[SP]);
    *address = gpr[PC];
    pom = gpr[regA] + gpr[regB] + disp;
    addr = reinterpret_cast<uint32_t*>(static_cast<char*>(mem) + pom);
    jumpAddress = *addr;
    gpr[PC] = jumpAddress;
  } else {
    cout << "Greska prilikom obrade instrukcije poziva podprograma" << endl;
    return -1;
  }
  return 1;
}
int Emulator::shiftInstruction(int mod, int regA, int regB, int regC) {
  if(mod == 0) {
    gpr[regA] = gpr[regB] << gpr[regC];
  } else if(mod == 1) {
    gpr[regA] = gpr[regB] >> gpr[regC];
  } else {
    cout << "Greska prilikom obrade aritmeticke instrukcije" << endl;
    return -1;
  }
  return 1;
}
int Emulator::jumpInstruction(int mod, int regA, int regB, int regC, int disp) {
  // Prve 4 vrednosti za MOD su kada je pomeraj do adrese skoka ukodiran u instrukciju
  // Druge 4 vrednosti su kada se adresa skoka dobija iz bazena literala
  unsigned int pom;
  uint32_t* addr;
  uint32_t jumpAddress;

  if(mod == 0) {
    //pc<=gpr[A]+D
    gpr[PC] = gpr[regA] + disp;
  } else if(mod == 1) {
    //if(gpr[B] == gpr[C]) pc<=gpr[A]+D 
    if(gpr[regB] == gpr[regC]) {
      gpr[PC] = gpr[regA] + disp;
    }
  } else if(mod == 2) {
    //if(gpr[B] != gpr[C]) pc<=gpr[A]+D 
    if(gpr[regB] != gpr[regC]) {
      gpr[PC] = gpr[regA] + disp;
    }
  } else if(mod == 3) {
    //if(gpr[B] signed> gpr[C]) pc<=gpr[A]+D
    if(gpr[regB] > gpr[regC]) {
      gpr[PC] = gpr[regA] + disp;
    }
  } else if(mod == 8) {
    //pc<=mem32[gpr[A]+D]
    pom = gpr[regA] + disp;
    addr = reinterpret_cast<uint32_t*>(static_cast<char*>(mem) + pom);
    jumpAddress = *addr;
    gpr[PC] = jumpAddress;
  } else if(mod == 9) {
    //if(gpr[B] == gpr[C]) pc<=mem32[gpr[A]+D]
    if(gpr[regB] == gpr[regC]) {
      pom = gpr[regA] + disp;
      addr = reinterpret_cast<uint32_t*>(static_cast<char*>(mem) + pom);
      gpr[PC] = *addr;
    }
  } else if(mod == 10) {
    //if(gpr[B] != gpr[C]) pc<=mem32[gpr[A]+D]
    if(gpr[regB] != gpr[regC]) {
      pom = gpr[regA] + disp;
      addr = reinterpret_cast<uint32_t*>(static_cast<char*>(mem) + pom);
      jumpAddress = *addr;
      gpr[PC] = jumpAddress;
    }
  } else if(mod == 11) {
    //if(gpr[B] > gpr[C]) pc<=mem32[gpr[A]+D]
    if(gpr[regB] > gpr[regC]) {
      pom = gpr[regA] + disp;
      addr = reinterpret_cast<uint32_t*>(static_cast<char*>(mem) + pom);
      jumpAddress = *addr;
      gpr[PC] = jumpAddress;
    }
  } else {
    cout << "Greska prilikom obrade instrukcije skoka" << endl;
    return -1;
  }

  return 1;
}
int Emulator::storeInstruction(int mod, int regA, int regB, int regC, int disp) {
  uint32_t pom;
  uint32_t* address;
  if(mod == 0) {
    //mem32[gpr[A]+gpr[B]+D]<=gpr[C];
    pom = gpr[regA] + gpr[regB] + disp;
    address = (uint32_t*)((char*)(mem) + pom);
    *address = gpr[regC];
  } else if(mod == 2) {
    //mem32[mem32[gpr[A]+gpr[B]+D]]<=gpr[C];
    pom = gpr[regA] + gpr[regB] + disp;
    address = (uint32_t*)((char*)(mem) + pom);
    uint32_t finalAddress = *address;
    address = (uint32_t*)((char*)(mem) + finalAddress);
    *address = gpr[regC];
  } else if(mod == 1) {
    //gpr[A]<=gpr[A]+D; mem32[gpr[A]]<=gpr[C];
    gpr[regA] = gpr[regA] + (int)disp;
    address = (uint32_t*)((char*)(mem) + gpr[regA]);
    *address = gpr[regC];
  } else {
    cout << "Greska prilikom obrade instrukcije STORE" << endl;
    return -1;
  }
  return 1; 
}
int Emulator::loadInstruction(int mod, int regA, int regB, int regC, int disp) {
    if(mod == 0) { 
      //gpr[A]<=csr[B]
      gpr[regA] = csr[regB];
    } else if(mod == 1) {
      //gpr[A]<=gpr[B]+D
      gpr[regA] = gpr[regB] + disp;
    } else if(mod == 2) {
      //gpr[A]<=mem32[gpr[B]+gpr[C]+D]
      uint32_t pom = gpr[regB] + gpr[regC] + disp;
      uint32_t* address = (uint32_t*)((char*)(mem) + pom);
      unsigned int val = *address;
      gpr[regA] = val;
    } else if(mod == 3) {
      //gpr[A]<=mem32[gpr[B]]; gpr[B]<=gpr[B]+D
      uint32_t* address = (uint32_t*)((char*)(mem) + gpr[regB]);
      unsigned int val = *address;
      gpr[regA] = val;
      gpr[regB] = gpr[regB] + disp;
    } else if(mod == 4) {
      //csr[A]<=gpr[B]
      csr[regA] = gpr[regB];
    } else if(mod == 5) {
      //csr[A]<=csr[B]|D
      csr[regA] = csr[regB] | disp;
    } else if(mod == 6) {
      //csr[A]<=mem32[gpr[B]+gpr[C]+D]
      uint32_t pom = gpr[regB] + gpr[regC] + disp;
      uint32_t* address = (uint32_t*)((char*)(mem) + pom);
      unsigned int val = *address;
      csr[regA] = val;
    } else if(mod == 7) {
      //csr[A]<=mem32[gpr[B]]; gpr[B]<=gpr[B]+D;
      uint32_t* address = (uint32_t*)((char*)(mem) + gpr[regB]);
      unsigned int val = *address;
      csr[regA] = val;
      gpr[regB] = gpr[regB] + disp;
    } else {
      cout << "Greska prilikom obrade instrukcije LOAD" << endl;
      return -1;
    }
  return 1;
}
int Emulator::intInstruction() {
  //push status; push pc; cause<=4; status<=status&(~0x1); pc<=handle;

  gpr[SP] = gpr[SP] - 4;
  uint32_t* address = (uint32_t*)((char*)(mem) + gpr[SP]);
  *address = csr[STATUS];
  gpr[SP] = gpr[SP] - 4;
  address = (uint32_t*)((char*)(mem) + gpr[SP]);
  *address = gpr[PC];
  csr[CAUSE] = 4;
  csr[STATUS] = csr[STATUS]&(~0x1);
  gpr[PC] = csr[HANDLER];
  return 1;
}

void Emulator::runEmulator() {
  gpr[PC] = 0x40000000;
  gpr[SP] = 0xFFFFFF00;
  while(!haltFlag) {
    uint32_t* addr = reinterpret_cast<uint32_t*>(static_cast<char*>(mem) + gpr[PC]);
    uint32_t instruction = *addr;
    gpr[PC] += 4; 
    //---------------------------------------------
    int oc = (instruction >> 28) & 0xF;
    int mod = (instruction >> 24) & 0xF;
    int regA = (instruction >> 20) & 0xF;
    int regB = (instruction >> 16) & 0xF;
    int regC = (instruction >> 12) & 0xF;
    int disp = instruction & 0xFFF;

    if(disp & 0x800) disp |= 0xFFFFF000;

    //cout << hex << "OC: " << oc << " mod: " << mod << " regA: " << regA << " regB: " << regB << " regC: " << regC << " disp: " << disp << endl;

    int result = 0;
    if(oc == 0) {
      //HALT
      haltFlag = true;
    } else if(oc == 1) {
      //INT
      result = intInstruction();
    } else if(oc == 2) {
      //CALL
      result = callInstruction(mod, regA, regB, regC, disp);
    }  else if(oc == 3) {
      //JUMPS
      result = jumpInstruction(mod, regA, regB, regC, disp);
    }else if(oc == 4) {
      //XCHG
      //temp<=gpr[B]; gpr[B]<=gpr[C]; gpr[C]<=temp;
      
      unsigned int tmp = gpr[regB];
      gpr[regB] = gpr[regC];
      gpr[regC] = tmp;
    }else if(oc == 5) {
      //ARITMETIC
      result = aritmeticInstruction(mod, regA, regB, regC, disp);
    } else if(oc == 6) {
      //LOGIC
      result = logicalInstruction(mod, regA, regB, regC);
    } else if(oc == 7) {
      //SHIFT
      result = shiftInstruction(mod, regA, regB, regC);
    } else if(oc == 8) {
      //STORE
      result = storeInstruction(mod, regA, regB, regC, disp);
    } else if(oc == 9) {
      //LOAD
      result = loadInstruction(mod, regA, regB, regC, disp);
    }

    if(result == -1) {
      cout << "GRESKA, emuliranje programa nije uspesno izvrseno!" << endl;
      return;
    }
  }
}

void Emulator::writeRegisters() {
  cout << "-------------------------------------------------------------------------------" << endl;
  cout << "Emulated processor executed halt instruction" << endl;
  cout << "Emulated processor state:" << endl;

  for(int i = 0; i < 16; i++) {
    cout << left << "r" << setw(2) << i << right << " = 0x" << hex << setw(8) << setfill('0') << gpr[i] << setfill(' ') << " " << dec;
    if((i + 1) % 4 == 0) cout << endl;
  }

  // Oslobadjanje memorije
  munmap(mem, (1UL << 32));
}







