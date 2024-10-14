#include "../inc/assembler.hpp"
#include <iostream>
#include <string>
#include <fstream>
#include <iomanip>


bool Assembler::isSymbol(string operand) {
  if(operand.rfind("0x", 0) == 0 || operand.rfind("0X", 0) == 0) return false;
  else if(!isalpha(operand[0])) return false;
  return true;
}
Assembler::Symbol Assembler::findSymbolInSymbolTable(string operand) {
  for (auto& sym : symbolTable) {
    if (sym.symbol == operand) return sym;
  }
  return Symbol(-1, -1, Assembler::SymbolType::NOTYPE, Assembler::Binding::LOCAL_SYM, -1, "", false);
}
void Assembler::updateCurrentSectionInList(BinaryLine newLine) {
  newLine.address = currentSection.size;
  currentSection.sectionLines.push_back(newLine);
  currentSection.size += 4;
  for(auto& sec: sections) {
    if(sec.sectionName == currentSection.sectionName) {
      sec = currentSection;
      break;
    }
  }
}

int Assembler::handleJumps(Line line, char oc, char firstMod, char secondMod) {
  if(isSymbol(line.operand)) {
    //OPERAND = SIMBOL  
    Symbol sym = findSymbolInSymbolTable(line.operand);
    if(sym.symbol != "") {
      //Simbol postoji u tabeli simbola
      if(sym.section == currentSection.sectionNumber && sym.isDefined) {
        //Simbol se nalazi u istoj sekciji kao i odogovarajuca naredba
        int oc_mod = (oc << 4) | (firstMod & 0xF);
        int pom =  sym.value - currentSection.size - 4;
        int regA_regB = (15 << 4) | (line.regA & 0xF);
        int regC_disp0 = (line.regB << 4) | ((pom >> 8) & 0xF);
        int disp1_disp2 = pom & 0xFF;
        BinaryLine newLine(oc_mod, regA_regB, regC_disp0, disp1_disp2, false);
        updateCurrentSectionInList(newLine);
      } else { 
        int oc_mod = (oc << 4) | (secondMod & 0xF);
        int regA_regB = (15 << 4) | (line.regA & 0xF);
        int regC_disp0 = (line.regB << 4) & 0xF0;
        BinaryLine newLine(oc_mod, regA_regB, regC_disp0, 0b00000000, false);
        //BEKPECING
        BackpatchElem newElem(currentSection.sectionNumber, line.operand, currentSection.size);
        currentSection.backpatch.push_back(newElem);
        updateCurrentSectionInList(newLine);
      }
    } else {
      //Simbol ne postoji u tabeli simbola
      int oc_mod = (oc << 4) | (secondMod & 0xF);
      int regA_regB = (15 << 4) | (line.regA & 0xF);
      int regC_disp0 = (line.regB << 4) & 0xF0;
      BinaryLine newLine(oc_mod, regA_regB, regC_disp0, 0b00000000, false);
      //BEKPECING
      BackpatchElem newElem(currentSection.sectionNumber, line.operand, currentSection.size);
      currentSection.backpatch.push_back(newElem);
      updateCurrentSectionInList(newLine);
    }
  } else {
    //OPERAND = LITERAL
    long operand;
    if(line.operand.rfind("0x", 0) == 0 || line.operand.rfind("0X", 0) == 0) {
      operand = stol(line.operand, NULL, 16);
    } else operand = stol(line.operand, NULL, 10);
    if(operand > (1UL << 32)) {
      cout << "Literal nije moguce zapisati na sirini od 32b." << endl;
      return -1;
    }
    if(operand < (1 << 12)) {
      int oc_mod = (oc << 4) | (firstMod & 0xF);
      int regA_regB = (15 << 4) | (line.regA & 0xF);
      int regC_disp0 = (line.regB << 4) | ((operand >> 8) & 0xF);
      int disp1_disp2 = operand & 0xFF;
      BinaryLine newLine(oc_mod, regA_regB, regC_disp0, disp1_disp2, false);
      updateCurrentSectionInList(newLine);
    } else {
      //Operand nije u neposrednoj blizini
      int oc_mod = (oc << 4) | (secondMod & 0xF);
      int regA_regB = (15 << 4) | (line.regA & 0xF);
      int regC_disp0 = (line.regB << 4) & 0xF0;
      BinaryLine newLine(oc_mod, regA_regB, regC_disp0, 0b00000000, false);
      //BEKPECING
      BackpatchElem newElem(currentSection.sectionNumber, line.operand, currentSection.size);
      currentSection.backpatch.push_back(newElem);
      updateCurrentSectionInList(newLine);
    }
  }
  return 1;
}
int Assembler::handleCALL(Line line) {
  if(isSymbol(line.operand)) {
    Symbol sym = findSymbolInSymbolTable(line.operand);
    if(sym.symbol != "") {
      //Simbol postoji u tabeli simbola
      if(sym.section == currentSection.sectionNumber && sym.isDefined) {
        //Simbol se nalazi u istoj sekciji kao i naredba CALL i njegova vrednost je poznata
        int pom =  sym.value - currentSection.size - 4;
        int regC_disp0 = (0 << 4) | ((pom >> 8) & 0xF);
        int disp1_disp2 = pom & 0xFF;
        BinaryLine newLine(0b00100000, 0b11110000, regC_disp0, disp1_disp2, false);
        updateCurrentSectionInList(newLine);
      } else {
        BinaryLine newLine(0b00100001, 0b11110000, 0b00000000, 0b00000000, false);
        //BEKPECING
        BackpatchElem newElem(currentSection.sectionNumber, line.operand, currentSection.size);
        currentSection.backpatch.push_back(newElem);
        updateCurrentSectionInList(newLine);
      }
    } else {
      //Simbol ne postoji u tabeli simbola
      BinaryLine newLine(0b00100001, 0b11110000, 0b00000000, 0b00000000, false);
      //BEKPECING
      BackpatchElem newElem(currentSection.sectionNumber, line.operand, currentSection.size);
      currentSection.backpatch.push_back(newElem);
      updateCurrentSectionInList(newLine);
    }
  } else {
    //OPERAND = LITERAL
    long operand;
    if(line.operand.rfind("0x", 0) == 0 || line.operand.rfind("0X", 0) == 0) {
      operand = stol(line.operand, NULL, 16);
    } else operand = stol(line.operand, NULL, 10);
    if(operand > (1UL << 32)) {
      cout << "Literal nije moguce zapisati na sirini od 32b" << endl;
      return -1;
    }
    if(operand < (1 << 12)) {
      int regC_disp0 = (0 << 4) | ((operand >> 8) & 0xF);
      int disp1_disp2 = operand & 0xFF;
      BinaryLine newLine(0b00100000, 0b00000000, regC_disp0, disp1_disp2, false);
      updateCurrentSectionInList(newLine);
    } else {
      BinaryLine newLine(0b00100001, 0b11110000, 0b00000000, 0b00000000, false);
      //BEKPECING
      BackpatchElem newElem(currentSection.sectionNumber, line.operand, currentSection.size);
      currentSection.backpatch.push_back(newElem);
      updateCurrentSectionInList(newLine);
    }
  }
  return 1;
}
int Assembler::handleJMP(Line line) {
  return handleJumps(line, 3, 0, 8);
}
int Assembler::handleBEQ(Line line) {
  return handleJumps(line, 3, 1, 9);
}
int Assembler::handleBNE(Line line) {
  return handleJumps(line, 3, 2, 10);
}
int Assembler::handleBGT(Line line) {
  return handleJumps(line, 3, 3, 11);
}

int Assembler::handleHALT(Line line) {
  BinaryLine newLine(0b00000000, 0b00000000, 0b00000000, 0b00000000, false);
  updateCurrentSectionInList(newLine);
  return 1;
}
int Assembler::handleINT(Line line) {
  BinaryLine newLine(0b00010000, 0b00000000, 0b00000000, 0b00000000, false);
  updateCurrentSectionInList(newLine);
  return 1;
}
int Assembler::handleIRET(Line line) {
  //pop status;
  int regA_regB = (0 << 4) | 14;
  BinaryLine newLine1(0b10010110, regA_regB, 0b00000000, 0b00000100, false);
  updateCurrentSectionInList(newLine1);
  //pop pc;
  regA_regB = (15 << 4) | 14;
  BinaryLine newLine2(0b10010011, regA_regB, 0b00000000, 0b0001000, false);
  updateCurrentSectionInList(newLine2);
  return 1;
}
int Assembler::handleRET(Line line) {
  //pop pc;
  int regA_regB = (15 << 4) | 14;
  BinaryLine newLine(0b10010011, regA_regB, 0b00000000, 0b00000100, false);
  updateCurrentSectionInList(newLine);
  return 1;
}
int Assembler::handlePUSH(Line line) {
  //sp <= sp - 4; mem32[sp] <= gpr;
  int regA_regB = (14 << 4) | 0;
  int regC_disp0 = (line.regA << 4) | 0xF;
  BinaryLine newLine(0b10000001, regA_regB, regC_disp0, 0b11111100, false);
  updateCurrentSectionInList(newLine);
  return 1;
}
int Assembler::handlePOP(Line line) {
  //gpr <= mem32[sp]; sp <= sp + 4;
  int regA_regB = (line.regA << 4) | 14;
  BinaryLine newLine(0b10010011, regA_regB, 0b00000000, 0b00000100, false);
  updateCurrentSectionInList(newLine);
  return 1;
}
int Assembler::handleXCHG(Line line) {
  int regA_regB = (0 << 4) | line.regB;
  int regC_disp0 = (line.regC << 4);
  BinaryLine newLine(0b01000000, regA_regB, regC_disp0, 0b00000000, false);
  updateCurrentSectionInList(newLine);
  return 1;
}
int Assembler::handleADD(Line line) {
  int regA_regB = (line.regA << 4) | (line.regB & 0xF); 
  int regC_disp0 = (line.regC << 4);
  BinaryLine newLine(0b01010000, regA_regB, regC_disp0, 0b00000000, false);
  updateCurrentSectionInList(newLine);
  return 1;
}
int Assembler::handleSUB(Line line) {
  int regA_regB = (line.regA << 4) | (line.regB & 0xF);
  int regC_disp0 = (line.regC << 4);
  BinaryLine newLine(0b01010001, regA_regB, regC_disp0, 0b00000000, false);
  updateCurrentSectionInList(newLine);
  return 1;
}
int Assembler::handleMUL(Line line) {
  int regA_regB = (line.regA << 4) | (line.regB & 0xF);
  int regC_disp0 = (line.regC << 4);
  BinaryLine newLine(0b01010010, regA_regB, regC_disp0, 0b00000000, false);
  updateCurrentSectionInList(newLine);
  return 1;
}
int Assembler::handleDIV(Line line) {
  int regA_regB = (line.regA << 4) | (line.regB & 0xF);
  int regC_disp0 = (line.regC << 4);
  BinaryLine newLine(0b01010011, regA_regB, regC_disp0, 0b00000000, false);
  updateCurrentSectionInList(newLine);
  return 1;
}
int Assembler::handleNOT(Line line) {
  int regA_regB = (line.regA << 4) | (line.regB & 0xF);
  BinaryLine newLine(0b01100000, regA_regB, 0b00000000, 0b00000000, false);
  updateCurrentSectionInList(newLine);
  return 1;
}
int Assembler::handleAND(Line line) {
  int regA_regB = (line.regA << 4) | (line.regB & 0xF);
  int regC_disp0 = (line.regC << 4);
  BinaryLine newLine(0b01100001, regA_regB, regC_disp0, 0b00000000, false);
  updateCurrentSectionInList(newLine);
  return 1;
}
int Assembler::handleOR(Line line) {
  int regA_regB = (line.regA << 4) | (line.regB & 0xF);
  int regC_disp0 = (line.regC << 4);
  BinaryLine newLine(0b01100010, regA_regB, regC_disp0, 0b00000000, false);
  updateCurrentSectionInList(newLine);
  return 1;
}
int Assembler::handleXOR(Line line) {
  int regA_regB = (line.regA << 4) | (line.regB & 0xF);
  int regC_disp0 = (line.regC << 4);
  BinaryLine newLine(0b01100011, regA_regB, regC_disp0, 0b00000000, false);
  updateCurrentSectionInList(newLine);
  return 1;
}
int Assembler::handleSHL(Line line) {
  int regA_regB = (line.regA << 4) | (line.regB & 0xF);
  int regC_disp0 = (line.regC << 4);
  BinaryLine newLine(0b01110000, regA_regB, regC_disp0, 0b00000000, false);
  updateCurrentSectionInList(newLine);
  return 1;
}
int Assembler::handleSHR(Line line) {
  int regA_regB = (line.regA << 4) | (line.regB & 0xF);
  int regC_disp0 = (line.regC << 4);
  BinaryLine newLine(0b01110001, regA_regB, regC_disp0, 0b00000000, false);
  updateCurrentSectionInList(newLine);
  return 1;
}
int Assembler::handleCSRRD(Line line) {
  int regA_regB = (line.regA << 4) | (line.regB & 0xF);
  BinaryLine newLine(0b10010000, regA_regB, 0b00000000, 0b00000000, false);
  updateCurrentSectionInList(newLine);
  return 1;
}
int Assembler::handleCSRWR(Line line) {
  int regA_regB = (line.regA << 4) | (line.regB & 0xF);
  BinaryLine newLine(0b10010100, regA_regB, 0b00000000, 0b00000000, false);
  updateCurrentSectionInList(newLine);
  return 1;
}

int Assembler::loadMEM_DIR(Line line) {
  if(isSymbol(line.operand)) {
    //OPERAND = SIMBOL 
    Symbol sym = findSymbolInSymbolTable(line.operand);
    if(sym.symbol != "") {
      //Simbol postoji u tabeli simbola
      if(sym.section == currentSection.sectionNumber && sym.isDefined) {
        //Simbol se nalazi u istoj sekciji kao i naredba, racunamo pomeraj od naredne instrukcije do simbola i to ukodiramo u instrukciju koja se obradjuje
        int pom =  sym.value - currentSection.size - 4;
        int regA_regB = (line.regA << 4) | (15 & 0xF);
        int regC_disp0 = (0 << 4) | ((pom >> 8) & 0xF);
        int disp1_disp2 = pom & 0xFF;
        BinaryLine newLine(0b10010001, regA_regB, regC_disp0, disp1_disp2, false);
        updateCurrentSectionInList(newLine);
      } else {
        //Dodajemo vrednost simbola u bazen literala, pa da bi dosli do vrednosti simbola u bazenu literala treba da saberemo vrednos PC registra sa pomerajem 
        //kako bismo dosli do odgovarajuceg elementa u bazenu literala i onda sa te adrese procitamo vrednost simbola
        int regA_regB = (line.regA << 4) | (15 & 0xF);
        BinaryLine newLine(0b10010010, regA_regB, 0b00000000, 0b00000000, false);
        //BEKPECING
        BackpatchElem newElem(currentSection.sectionNumber, line.operand, currentSection.size);
        currentSection.backpatch.push_back(newElem);
        updateCurrentSectionInList(newLine);
      }
    } else {
      //Simbol ne postoji u tabeli simbola
      //Vrednost se cita iz bazena literala
      int regA_regB = (line.regA << 4) | (15 & 0xF);
      BinaryLine newLine(0b10010010, regA_regB, 0b00000000, 0b00000000, false);
      //BEKPECING
      BackpatchElem newElem(currentSection.sectionNumber, line.operand, currentSection.size);
      currentSection.backpatch.push_back(newElem);
      updateCurrentSectionInList(newLine);
    }
  } else {
    //OPERAND = LITERAL
    long operand;
    if(line.operand.rfind("0x", 0) == 0 || line.operand.rfind("0X", 0) == 0) {
      operand = stol(line.operand, NULL, 16);
    } else operand = stol(line.operand, NULL, 10);
    if(operand > (1UL << 32)) {
      cout << "Literal nije moguce zapisati na sirini od 32b" << endl;
      return -1;
    }
    if(operand < (1 << 12)) {
      int regA_regB = (line.regA << 4) & 0xF0;
      int regC_disp0 = (0 << 4) | ((operand >> 8) & 0xF);
      int disp1_disp2 = operand & 0xFF;
      BinaryLine newLine(0b10010001, regA_regB, regC_disp0, disp1_disp2, false);
      updateCurrentSectionInList(newLine);
    } else {
      //Ova konstanta se nalazi u bazenu literala, i one se nalazi na adresi mem32[pc + pom]
      int regA_regB = (line.regA << 4) | (15 & 0xF);
      BinaryLine newLine(0b10010010, regA_regB, 0b00000000, 0b00000000, false);
      //BEKPECING
      BackpatchElem newElem(currentSection.sectionNumber, line.operand, currentSection.size);
      currentSection.backpatch.push_back(newElem);
      updateCurrentSectionInList(newLine);
    }
  }
  return 1;
}
int Assembler::handleLD(Line line) {
  if(line.addresingMode == MEM_DIR) {
    // $operand
    return loadMEM_DIR(line);
  } else if(line.addresingMode == MEM_INDIR) {
    // operand
    int regA = loadMEM_DIR(line);
    if(regA != -1){
      //U line.regA se sada nalazi adresa sa koje treba ucitati nesto 
      int regA_regB = (line.regA << 4) | (line.regA & 0xF);
      BinaryLine newLine(0b10010010, regA_regB, 0b00000000, 0b00000000, false);
      updateCurrentSectionInList(newLine);
      return 1;
    } else return -1;
  } else if(line.addresingMode == REG_DIR) {
    int regA_regB = (line.regA << 4) | (line.regB & 0xF);
    BinaryLine newLine(0b10010001, regA_regB, 0b00000000, 0b00000000, false);
    updateCurrentSectionInList(newLine);
    return 1;
  } else if(line.addresingMode == REG_INDIR) {
    int regA_regB = (line.regA << 4) | (line.regB & 0xF);
    BinaryLine newLine(0b10010010, regA_regB, 0b00000000, 0b00000000, false);
    updateCurrentSectionInList(newLine);
    return 1;
  } else if(line.addresingMode == REG_INDIR_POM) {
    if(!isSymbol(line.operand)) {
      //OPERAND = LITERAL
      long operand;
      if(line.operand.rfind("0x", 0) == 0 || line.operand.rfind("0X", 0) == 0) {
        operand = stol(line.operand, NULL, 16);
      } else operand = stol(line.operand, NULL, 10);
      if(operand > (1 << 12)) {
        cout << "Literal nije moguce zapisati na sirini od 32b" << endl;
        return -1;
      }

      int regA_regB = (line.regA << 4) | (line.regB & 0xF);
      int regC_disp0 = (0 << 4) | ((operand >> 8) & 0xF);
      int disp1_disp2 = operand & 0xFF;
      BinaryLine newLine(0b10010010, regA_regB, regC_disp0, disp1_disp2, false);
      updateCurrentSectionInList(newLine);
      return 1;
    } else {
      //OPERAND = SYMBOL
      return 1;
    }
  } 
  return -1;
}

int Assembler::storeMEM_DIR_INDIR(Line line, char mod) {
  int oc_mod = (8 << 4) | (mod & 0xF);
  if(isSymbol(line.operand)){
    //OPERAND = SYMBOL
    Symbol sym = findSymbolInSymbolTable(line.operand);
    if(sym.symbol != "") {
      //Simbol postoji u tabeli simbola
      if(sym.section == currentSection.sectionNumber && sym.isDefined) {
        //Simbol se nalazi u istoj sekciji kao i naredba
        int pom =  sym.value - currentSection.size - 4;
        int regC_disp0 = (line.regA << 4) | ((pom >> 8) & 0xF);
        int disp1_disp2 = pom & 0xFF;
        BinaryLine newLine(oc_mod, 0b11110000, regC_disp0, disp1_disp2, false);
        updateCurrentSectionInList(newLine);
      } else {
        int regC_disp0 = (line.regA << 4);
        BinaryLine newLine(oc_mod, 0b11110000, regC_disp0, 0b00000000, false);
        //BEKPECING
        BackpatchElem newElem(currentSection.sectionNumber, line.operand, currentSection.size);
        currentSection.backpatch.push_back(newElem);
        updateCurrentSectionInList(newLine);
      }
    } else {
      //Simbol ne postoji u tabeli simbola
      int regC_disp0 = (line.regA << 4);
      BinaryLine newLine(oc_mod, 0b11110000, regC_disp0, 0b00000000, false);
      //BEKPECING
      BackpatchElem newElem(currentSection.sectionNumber, line.operand, currentSection.size);
      currentSection.backpatch.push_back(newElem);
      updateCurrentSectionInList(newLine);
    }
  } else {
    //OPERAND = LITERAL
    long operand;
    if(line.operand.rfind("0x", 0) == 0 || line.operand.rfind("0X", 0) == 0) {
      operand = stol(line.operand, NULL, 16);
    } else operand = stol(line.operand, NULL, 10);
    if(operand > (1UL << 32)) {
      cout << "Literal nije moguce zapisati na sirini od 32b" << endl;
      return -1;
    }
    if(operand < (1 << 12)) {
      int regC_disp0 = (line.regA << 4) | ((operand >> 8) & 0xF);
      int disp1_disp2 = operand & 0xFF;
      BinaryLine newLine(oc_mod, 0b00000000, regC_disp0, disp1_disp2, false);
      updateCurrentSectionInList(newLine);
      return 1;
    } else {
      int regC_disp0 = (line.regA << 4);
      BinaryLine newLine(oc_mod, 0b11110000, regC_disp0, 0b00000000, false);
      //BEKPECING
      BackpatchElem newElem(currentSection.sectionNumber, line.operand, currentSection.size);
      currentSection.backpatch.push_back(newElem);
      updateCurrentSectionInList(newLine);
      return 1;
    }
  }
  return 1;
}
int Assembler::handleST(Line line) {
  if(line.addresingMode == MEM_DIR) {
    return storeMEM_DIR_INDIR(line, 0);
  } else if (line.addresingMode == MEM_INDIR) {
    return storeMEM_DIR_INDIR(line, 2);
  } else if (line.addresingMode == REG_DIR) {
    int regA_regB = (line.regA << 4) | (line.regB & 0xF); 
    BinaryLine newLine(0b10010001, regA_regB, 0b00000000, 0b00000000, false);
    updateCurrentSectionInList(newLine);
    return 1;
  } else if (line.addresingMode == REG_INDIR) {
    int regA_regB = (line.regA << 4);
    int regC_disp0 = (line.regB << 4);
    BinaryLine newLine(0b10000000, regA_regB, regC_disp0, 0b00000000, false);
    updateCurrentSectionInList(newLine);
    return 1;
  } else if (line.addresingMode == REG_INDIR_POM) {
    if(!isSymbol(line.operand)) {
      //OPERAND = LITERAL
      long operand;
      if(line.operand.rfind("0x", 0) == 0 || line.operand.rfind("0X", 0) == 0) {
        operand = stol(line.operand, NULL, 16);
      } else operand = stol(line.operand, NULL, 10);
      if(operand > (1 << 12)) {
        cout << "Literal je veci od 12b. Nije moguce izvrsiti funkciju ST." << endl;
        return -1;
      }

      int regA_regB = (line.regA << 4);
      int regC_disp0 = (line.regB << 4) | ((operand >> 8) & 0xF);
      int disp1_disp2 = operand & 0xFF;
      BinaryLine newLine(0b10000000, regA_regB, regC_disp0, disp1_disp2, false);
      updateCurrentSectionInList(newLine);
      return 1;
    } else {
      //OPERAND = SYMBOL
      return 1;
    }
  }
  return 1;
}

int Assembler::handleGLOBAL(Line line) {
  if(line.listOfSymbols.empty()) {
    cout << "U listi simbola direktive .GLOBAL nista nije navedeno." << endl;
    return -1;
  }
  for(string sym: line.listOfSymbols) {
    Symbol s = findSymbolInSymbolTable(sym); 
    if(s.symbol != "") {
      s.bind = Binding::GLOBAL_SYM;
    } else {
      Symbol newSymbol(0, 0, SymbolType::NOTYPE, Binding::GLOBAL_SYM, currentSection.sectionNumber, sym, false);
      symbolTable.push_back(newSymbol);
    }
  }
  return 1;
}
int Assembler::handleEXTERN(Line line) {
  if(line.listOfSymbols.empty()) {
    cout << "U listi simbola direktive .EXTERN nista nije navedeno." << endl; 
    return -1;
  }
  for(string sym: line.listOfSymbols) {
    Symbol s = findSymbolInSymbolTable(sym); 
    if(s.symbol != "") {



      return -1;
    } else {
      Symbol newSymbol(0, 0, SymbolType::NOTYPE, Binding::GLOBAL_SYM, -1, sym, false);
      symbolTable.push_back(newSymbol);
    }
  }
  return 1;
}
int Assembler::handleSECTION(Line line) {
  Symbol sec = findSymbolInSymbolTable(line.operand);
  if(sec.symbol != "") {
    //Simbol pod ovim imenom vec postoji
    if(sec.type != SymbolType::SCTN) {
      cout << "Simbol postoji u tabeli simbola, ali nije naziv sekcije." << endl;
      return -1;
    }
    for(auto& s: sections) {
      if(s.sectionName == sec.symbol) {
        currentSection = s;
        break;
      }
    }
    return 1;
  } else {
    currentSection = Section(line.operand);
    currentSection.sectionNumber = numOfCurrentSection++;
    sections.push_back(currentSection);

    Symbol newSymbol(0, 0, SymbolType::SCTN, Binding::LOCAL_SYM, currentSection.sectionNumber, line.operand, true);
    symbolTable.push_back(newSymbol);
  }
  return 1;
}
int Assembler::handleWORD(Line line) {
  if(line.listOfSymbols.empty()) {
    cout << "U listi simbola direktive .WORD nista nije navedeno." << endl;
    return -1;
  }
  for(string operand: line.listOfSymbols) {
    if(isSymbol(operand)) {
      //OPERAND = SYMBOL
      Symbol sym = findSymbolInSymbolTable(operand);
      if(sym.symbol != "" && sym.isDefined) {
        //Simbol postoji u tabeli simbola i znamo njegovu vrednost
        int oc_mod = (sym.value >> 24) & 0xFF;
        int regA_regB =  (sym.value >> 16) & 0xFF;
        int regC_disp0 = (sym.value >> 8) & 0xFF;
        int disp1_disp2 = sym.value & 0xFF;
        BinaryLine newLine(oc_mod, regA_regB, regC_disp0, disp1_disp2, true);
        updateCurrentSectionInList(newLine);
      } else {
        //Simbol ne postoji u tabeli simbola -> BEKPECING
        BinaryLine newLine(0b00000000, 0b00000000, 0b00000000, 0b00000000, false);
        //BEKPECING
        BackpatchElem newElem(currentSection.sectionNumber, line.operand, currentSection.size);
        currentSection.backpatch.push_back(newElem);
        updateCurrentSectionInList(newLine);
      }
    } else {
      //OPERAND = LITERAL
      long op;
      if(operand.rfind("0x", 0) == 0 || operand.rfind("0X", 0) == 0) {
        op = stol(operand, NULL, 16);
      } else op = stol(operand, NULL, 10);

      if(op <= (1UL << 32)) {
        int oc_mod = (op >> 24) & 0xFF;
        int regA_regB =  (op >> 16) & 0xFF;
        int regC_disp0 = (op >> 8) & 0xFF;
        int disp1_disp2 = op & 0xFF;
        BinaryLine newLine(oc_mod, regA_regB, regC_disp0, disp1_disp2, true);
        updateCurrentSectionInList(newLine);
      } else {
        cout << "Literal je veci od 32b." << endl;
        return -1;
      }
    }
  }
  return 1;
}
int Assembler::handleSKIP(Line line) {
  long literal;
  if(line.operand.rfind("0x", 0) == 0 || line.operand.rfind("0X", 0) == 0) {
    literal = stol(line.operand, NULL, 16);
  } else literal = stol(line.operand, NULL, 10);

  //Kad budem upisivala u binarni fajl prolazicu kroz listu i ako naidjem na BinaryLine koji je direktiva onda cu onoliko
  //puta kolika je vrednost za byteToSkip upisati 0b00000000
  BinaryLine newLine(literal, true);
  currentSection.sectionLines.push_back(newLine);
  currentSection.size += literal;
  for(auto& sec: sections) {
    if(sec.sectionName == currentSection.sectionName) {
      sec = currentSection;
      break;
    }
  }
  return 1;
}
int Assembler::handleEND(Line line) {
  assemblingFinished = true;
  return 1;
}
int Assembler::handleLABEL(Line line) {
  Symbol sym = findSymbolInSymbolTable(line.operand);
  if(sym.symbol != "") {
    //Simbol postoji u tabeli simbola
    if(!sym.isDefined) {
      sym.section = currentSection.sectionNumber;
      sym.value = currentSection.size;
      sym.isDefined = true;
      for(auto& sec: symbolTable) {
        if(sec.symbol == sym.symbol) {
          sec = sym;
          break;
        }
      }
    } else {
      cout << "Simbol je vec definisan i nije moguce ponovo ga definisati." << endl;
      return -1;
    }
  } else {
    //Simbol ne postoji u tabeli simbola
    Symbol newSymbol(currentSection.size, 0, SymbolType::NOTYPE, Binding::LOCAL_SYM, currentSection.sectionNumber, line.operand, true);
    symbolTable.push_back(newSymbol);
  }
  return 1;
}

Assembler::LiteralPoolElem Assembler::doesLiteralExistsInPool( vector<LiteralPoolElem> literalsPool, string literal) {
  for(auto& elem: literalsPool) {
    if(elem.literal == literal) return elem;
  }
  return LiteralPoolElem(-1, "", -1);
}
int Assembler::fixInstruction(LiteralPoolElem elem, Section& section, BackpatchElem bp, int positionOfLiteral) {
  // pom je pomeraj od instrukcije koju treba popraviti do mesta u bazenu literala gde se nalazi element koji se koristi u okviru instrukcije
  int pom;
  if(elem.redniBroj != -1) pom = section.size - bp.address + (elem.redniBroj - 1)* 4; //ako vec postoji u bazenu literala
  else pom = section.size - bp.address + (positionOfLiteral - 2) * 4;
  //Pronadjemo datu instrukciju i izmenimo je
  for(auto& bl: section.sectionLines){
    if(bl.address == bp.address) {
      int regC_disp0 = (bl.regC_disp0 & 0xF0) | ((pom >> 8) & 0xF);
      int disp1_disp2 = pom & 0xFF;
      bl.regC_disp0 = regC_disp0;
      bl.disp1_disp2 = disp1_disp2;
      return 1; 
    }
  }
  return 1;
}
int Assembler::backpatch() {
  int result = -1;

  for(auto& section: sections) {
    int positionOfLiteral = 0;

    for(auto& bp: section.backpatch) {
      if(!isSymbol(bp.operand)) {
        //LITERAL
        LiteralPoolElem elem = doesLiteralExistsInPool(section.literalsPool, bp.operand);
        if(elem.redniBroj == -1) {
          //Literal se ne nalazi u bazenu literala 
          long value;
          if(bp.operand.rfind("0x", 0) == 0 || bp.operand.rfind("0X", 0) == 0) {
            value = stol(bp.operand, NULL, 16);
          } else value = stol(bp.operand, NULL, 10);
          LiteralPoolElem newLiteral(positionOfLiteral++, bp.operand, value);
          section.literalsPool.push_back(newLiteral);
          //section.size += 4;
        }
        result = fixInstruction(elem, section, bp, positionOfLiteral); 
      } else {
        //SYMBOL
        Symbol sym = findSymbolInSymbolTable(bp.operand);
        if(sym.symbol == "" || !sym.isDefined && sym.bind != Assembler::Binding::GLOBAL_SYM) {
          cout << "Simbol ne postoji u tabeli simbola ili nije definisan. " << bp.operand << endl; 
          return -1;
        } else {
          /*if(sym.section == section.sectionNumber) {
            //Simbol postoji u tabeli simbola i u istoj je sekciji ---> Racunamo samo pomeraj do njega
            int pom = sym.value - bp.address - 4; 
            for(auto& bl: section.sectionLines){
              if(bl.address == bp.address) {
                int regC_disp0 = (bl.regC_disp0 & 0xF0) | ((pom >> 8) & 0xF);
                int disp1_disp2 = pom & 0xFF;
                bl.regC_disp0 = regC_disp0;
                bl.disp1_disp2 = disp1_disp2;
                result = 1;
                break; 
              }
            }
          } else {*/
            LiteralPoolElem elem = doesLiteralExistsInPool(section.literalsPool, bp.operand);
            if(elem.redniBroj == -1) {
              //Simbol ne postoji u bazenu literala
              LiteralPoolElem newPoolElem(positionOfLiteral++, sym.symbol, 0);
              section.literalsPool.push_back(newPoolElem);
              //elem = newPoolElem;
              if(sym.bind == Assembler::Binding::GLOBAL_SYM) {
                RelocationElem newRelaElem((section.size + (positionOfLiteral - 1) * 4), sym, sym.symbol, 0); 
                section.relocationTable.push_back(newRelaElem);
              } else {
                string sectionName = sections[sym.section].sectionName;
                RelocationElem newRelaElem((section.size + (positionOfLiteral - 1) * 4), sym, sectionName, sym.value); 
                section.relocationTable.push_back(newRelaElem);
              }
            }
            result = fixInstruction(elem, section, bp, positionOfLiteral);
        }
      }
    }
  }
  return 1;
}

void Assembler::createBinary() {
  std::ofstream file(nameOfFile, ios::binary | ios::out);
  if (!file) {
    throw std::runtime_error("Cannot open file for writing.");
  }

  //Zapisujemo informacije o tabeli sekcija
  int sectiojnTableSize = sections.size();
  file.write(reinterpret_cast<const char*>(&sectiojnTableSize), sizeof(sectiojnTableSize));
  int startAddres = 0;
  for(const auto& section : sections) {
    //Naziv sekcije
    int sectionNameLen = section.sectionName.length();
    file.write(reinterpret_cast<const char*>(&sectionNameLen), sizeof(sectionNameLen));
    file.write(section.sectionName.c_str(), sectionNameLen);
    //Pocetna adresa sekcije u fajlu
    file.write(reinterpret_cast<const char*>(&startAddres), sizeof(startAddres));
    //Velicina sekcije
    int size = section.size + section.literalsPool.size() * 4;
    file.write(reinterpret_cast<const char*>(&size), sizeof(size));
    startAddres += size; 
  }

  //Zapisujemo tabelu simbola
  int numSymbols = 0;
  for(int i = 0; i < symbolTable.size(); i++) {
    if(symbolTable[i].bind == Binding::GLOBAL_SYM || symbolTable[i].type == SymbolType::SCTN) numSymbols++;
  } 
  file.write(reinterpret_cast<const char*>(&numSymbols), sizeof(numSymbols));
  for (const auto& symbol : symbolTable) {
    if(symbol.bind == Binding::LOCAL_SYM && symbol.type != SymbolType::SCTN) continue;
    size_t symbolNameSize = symbol.symbol.size();
    file.write(reinterpret_cast<const char*>(&symbol.value), sizeof(symbol.value));
    file.write(reinterpret_cast<const char*>(&symbol.size), sizeof(symbol.size));
    file.write(reinterpret_cast<const char*>(&symbol.type), sizeof(symbol.type));
    file.write(reinterpret_cast<const char*>(&symbol.bind), sizeof(symbol.bind));
    file.write(reinterpret_cast<const char*>(&symbol.section), sizeof(symbol.section));

    file.write(reinterpret_cast<const char*>(&symbolNameSize), sizeof(symbolNameSize));
    file.write(symbol.symbol.c_str(), symbolNameSize);
  }

  // Broj sekcija
  int s = sections.size();
  file.write(reinterpret_cast<const char*>(&s), sizeof(s));
  for (const auto& section : sections) {
    //Velicina sekcije
    int size = section.size + section.literalsPool.size() * 4;
    file.write(reinterpret_cast<const char*>(&size), sizeof(size));
    //Naziv sekcije
    int sectionNameLen = section.sectionName.length();
    file.write(reinterpret_cast<const char*>(&sectionNameLen), sizeof(sectionNameLen));
    file.write(section.sectionName.c_str(), sectionNameLen);

    for (const auto& line : section.sectionLines) {
      if(line.byteToSkip == -1) {
        uint8_t oc_mod = line.OC_MOD & 0xFF;
        uint8_t regA_regB = line.regA_regB & 0xFF;
        uint8_t regC_disp0 = line.regC_disp0 & 0xFF;
        uint8_t disp1_disp2 = line.disp1_disp2 & 0xFF;
        //cout << line.OC_MOD << " " << line.regA_regB << " " << line.regC_disp0 << " " << line.disp1_disp2 << endl ;

        file.write(reinterpret_cast<const char*>(&oc_mod), sizeof(oc_mod));
        file.write(reinterpret_cast<const char*>(&regA_regB), sizeof(regA_regB));
        file.write(reinterpret_cast<const char*>(&regC_disp0), sizeof(regC_disp0));
        file.write(reinterpret_cast<const char*>(&disp1_disp2), sizeof(disp1_disp2));
      } else {
        uint8_t zero = 0b00000000;
        for(int i = 0 ; i < line.byteToSkip; i++) {
         file.write(reinterpret_cast<const char*>(&zero), sizeof(zero));
        }
      }
    }

    // Zapisujemo bazen literala
    for (const auto& literal : section.literalsPool) {
      uint8_t byte1 = (literal.value >> 24) & 0xFF;
      uint8_t byte2 = (literal.value >> 16) & 0xFF;
      uint8_t byte3 = (literal.value >> 8) & 0xFF;
      uint8_t byte4 = literal.value & 0xFF;
      file.write(reinterpret_cast<const char*>(&byte1), sizeof(byte1));
      file.write(reinterpret_cast<const char*>(&byte2), sizeof(byte2));
      file.write(reinterpret_cast<const char*>(&byte3), sizeof(byte3));
      file.write(reinterpret_cast<const char*>(&byte4), sizeof(byte4));
    }

    // Velicina relokacione tabele
    int numRelocations = section.relocationTable.size();
    file.write(reinterpret_cast<const char*>(&numRelocations), sizeof(numRelocations));
    for (const auto& rel : section.relocationTable) {
      // offset
      file.write(reinterpret_cast<const char*>(&rel.offset), sizeof(rel.offset));
      // Simbol name 
      int symbolSize = rel.symbolName.size();
      file.write(reinterpret_cast<const char*>(&symbolSize), sizeof(symbolSize));
      file.write(rel.symbolName.c_str(), symbolSize);
      // Addend
      file.write(reinterpret_cast<const char*>(&rel.addend), sizeof(rel.addend));
    }
  }

  // Zatvorite fajl
  file.close();
}

void Assembler::createText() {
    size_t lastDot = nameOfFile.find_last_of('.');
    string name = nameOfFile.substr(0, lastDot);
    name = name + ".txt";
    std::ofstream file(name);
    if (!file) {
        throw std::runtime_error("Cannot open file for writing.");
    }
    // Ispis tabele sekcija
    file << "=============== Table of Sections ================\n";
    file << std::left << std::setw(25) << "Section Name" 
         << std::setw(20) << "Section Number" 
         << std::setw(20) << "Size" << "\n";
    file << std::string(50, '-') << "\n";
    for (const auto& section : sections) {
        file << std::left << std::setw(20) << section.sectionName
             << std::setw(20) << section.sectionNumber
             << std::setw(20) << (section.size + section.literalsPool.size() * 4) << "\n";
    }
    file << "\n";

    // Ispis tabele simbola
    file << "============================= Symbol Table ==============================\n";
    size_t numSymbols = symbolTable.size();
    file << "Number of Symbols: " << numSymbols << "\n";
    file << std::string(72, '-') << "\n";
    file << std::left << std::setw(15) << "Symbol" 
         << std::setw(10) << "Value" 
         << std::setw(8) << "Size"
         << std::setw(15) << "Type" 
         << std::setw(15) << "Bind" 
         << std::setw(15) << "Ndx" << "\n";
    file << std::string(72, '-') << "\n";
    for (const auto& symbol : symbolTable) {
        file << std::left << std::setw(15) << symbol.symbol
             << hex << std::setw(10) << symbol.value
             << dec << std::setw(8) << symbol.size
             << std::setw(15) << (symbol.type == SCTN ? "SCTN" : "NOTYP")
             << std::setw(15) << (symbol.bind == LOCAL_SYM ? "LOCAL" : "GLOBAL");
             if(symbol.section != -1) file << std::setw(15) << symbol.section << "\n";
             else file << std::setw(15) << "UND"<< "\n";
    }

    file << "\n";

    // Ispis linija sekcije
    file << "=================== Sections ===================\n";
    for (const auto& section : sections) {
        file << "========= Section binary =========\n";
        file << "Section Name: " << section.sectionName << "\n";
        file << "Section Number: " << dec << section.sectionNumber << "\n";
        file << "Size: " << dec << section.size  + section.literalsPool.size() * 4 << "\n";

        int addres = 0;
        for (const auto& line : section.sectionLines) {
            if (line.byteToSkip == -1) {
              if(addres % 8 == 0) file << setw(8) << setfill('0') << hex << addres << ": ";
              file << setw(2) << setfill('0') << hex <<  line.disp1_disp2 << " " << setw(2) << setfill('0') << line.regC_disp0 << " " << setw(2) << setfill('0') << line.regA_regB  << " " << setw(2) << setfill('0') << line.OC_MOD << " ";
              addres += 4;
              if(addres % 8 == 0) file << endl;
            } else {
              
            }
        }

        for (const auto& literal : section.literalsPool) {
            if(addres % 8 == 0) file << setw(8) << setfill('0') << hex << addres << ": ";
            file << setw(2) << setfill('0') << hex <<  (literal.value & 0xFF) << " " << setw(2) << setfill('0') << ((literal.value >> 8) & 0xFF) << " " << setw(2) << setfill('0') << ((literal.value >> 16) & 0xFF)  << " " << setw(2) << setfill('0') << ((literal.value >> 24) & 0xFF) << " ";
            addres += 4;
            if(addres % 8 == 0) file << endl;
        }
    }
    file << "\n";
    file << "\n";

    // Ispis tabele relokacija 
    file << "========== Relocation Tables ===========\n";
    file << std::setw(10) << setfill(' ') << "Offset"
        << std::setw(20) << "Symbol Name"
        << std::setw(10) << "Addend" << "\n";
    file << std::string(40, '-') << "\n";
    for(const auto& section: sections) {
      for (const auto& rel : section.relocationTable) {
        file << std::left << dec << std::setw(10) << rel.offset   
            << std::setw(20) << rel.symbolName
            << std::setw(10) << rel.addend << "\n";  
      }
    }
    file << "\n";

    // Zatvorite fajl
    file.close();
}



