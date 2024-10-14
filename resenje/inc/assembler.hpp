#ifndef _ASSEMBLER_HPP
#define _ASSEMBLER_HPP

#include <vector>
#include <string>
#include <ostream>
#include <iostream>

using namespace std;

class Assembler {
public: 
  enum Directive {GLOBAL, EXTERN, SECTION, WORD, SKIP, ASCII, EQU, END, LABEL, NO_DIR};

  enum Instruction {HALT, INT, IRET, CALL, RET, JMP, BEQ, BNE, BGT, PUSH, POP, XCHG, ADD, 
                  SUB, MUL, DIV, NOT, AND, OR, XOR, SHL, SHR, LD, ST, CSRRD, CSRWR, NO_INSTR};

  enum AddressingMode {MEM_DIR, MEM_INDIR, REG_DIR, REG_INDIR, REG_INDIR_POM, NO_ADDRESSING_MODE};

  enum Binding {LOCAL_SYM, GLOBAL_SYM, ERR_BIND};

  enum SymbolType {SCTN, NOTYPE, ERR_TYPE};

  struct Line {
    AddressingMode addresingMode = NO_ADDRESSING_MODE;
    Instruction instruction = NO_INSTR;
    int regA;
    int regB;
    int regC;

    string operand = "";   //operand instrukcije (simbol, literal)

    Directive directive = NO_DIR;
    vector<std::string> listOfSymbols;

    Line(AddressingMode addressingMode, Instruction instruction, int regA, int regB, int regC, string operand) {
      this->addresingMode = addressingMode;
      this->instruction = instruction;
      this->regA = regA;
      this->regB = regB;
      this->regC = regC;
      this->operand = operand;
    }
    Line(Directive directive, string operand, vector<string> listOfSymbols){
      this->directive = directive;
      this->operand = operand;
      this->listOfSymbols = listOfSymbols;
    }

  };

private:

  struct BinaryLine {
    int OC_MOD;
    int regA_regB;
    int regC_disp0;
    int disp1_disp2;

    bool isDirective = false;
    int byteToSkip = -1;

    int address = -1;

    BinaryLine(int OC_MOD, int regA_regB, int regC_disp0, int disp1_disp2, bool isDirective){
      this->OC_MOD= OC_MOD;
      this->regA_regB = regA_regB;
      this->regC_disp0 = regC_disp0;
      this->disp1_disp2 = disp1_disp2;
      this->isDirective = isDirective;
      this->address = address;
    }
    BinaryLine(int byteToSkip, bool isDirective){
      this->byteToSkip = byteToSkip;
      this->isDirective = isDirective;
    }
  };

  struct Symbol {
    int value;          //vrednost simbola(adresa na kojoj se taj simbol nalazi)
    int size;     
    SymbolType type;    //SCTN(kada je simbol naziv sekcije), NOTYP
    Binding bind;       //LOCAL,GLOBAL
    int section;        //kojoj sekciji simbol pripada (ndx)
    string symbol;      //naziv simbola
    bool isDefined;

    Symbol() {
      this->value = -1;
      this->size = -1;
      this->type = ERR_TYPE;
      this->bind = ERR_BIND;
      this->section = -1;
      this->symbol = "";
      this->isDefined = -1;
    }
    Symbol(int value, int size, SymbolType type, Binding bind, int section, string symbol, bool isDefine) {
      this->value = value;
      this->size = size;
      this->type = type;
      this->bind = bind;
      this->section = section;
      this->symbol = symbol;
      this->isDefined = isDefine;
    }
  };

  struct LiteralPoolElem {
    int redniBroj;
    string literal;
    int value;

    LiteralPoolElem(int redniBroj, string literal, int value) {
      this->redniBroj = redniBroj;
      this->literal = literal;
      this->value = value;
    }
  };

  struct BackpatchElem {
    int sectionNumber;
    string operand;
    int address;

    BackpatchElem(int sectionNumber, string operand, int address) {
      this->sectionNumber = sectionNumber;
      this->operand = operand;
      this->address = address;
    }
  };

  struct RelocationElem {
    int offset = 0;
    Symbol symbol;
    string symbolName = "";
    int addend = 0;

    //RelocationElem() : symbol(*(new SymbolTable())) {}
    RelocationElem(int offset, Symbol symbol, string symbolName, int addend) {
      this->offset = offset;
      this->symbolName = symbolName;
      this->symbol = symbol;
      this->addend = addend;
    }
  };

  struct Section {
    string sectionName = "";
    int sectionNumber = -1;
    int size = 0;
    vector<BinaryLine> sectionLines;
    vector<LiteralPoolElem> literalsPool;
    vector<BackpatchElem> backpatch;
    vector<RelocationElem> relocationTable;

    Section() {}

    Section(string sectionName) {
      this->sectionName = sectionName;
    }
  };
  
  vector<Line> progaramLines;       //Kada parser procita liniju, ona se doda u ovu listu
  vector<Section> sections;    //Lista sekcija 
  Section defSection = Section();
  Section& currentSection = defSection;      //Trenutna sekcija
  int numOfCurrentSection = 0;                  
  vector<Symbol> symbolTable;  //Lista simbola gde je cvor liste jedan red iz tabele simbola
  bool assemblingFinished = false;
  
public:
  string nameOfFile;
  Assembler(const char* n) {
    //this->nameOfFile = string(n);
  }

  int handleHALT(Line line);
  int handleINT(Line line);
  int handleIRET(Line line);
  int handleCALL(Line line);
  int handleRET(Line line);
  int handleJMP(Line line);
  int handleBEQ(Line line);
  int handleBNE(Line line);
  int handleBGT(Line line);
  int handlePUSH(Line line);
  int handlePOP(Line line);
  int handleXCHG(Line line);
  int handleADD(Line line);
  int handleSUB(Line line);
  int handleMUL(Line line);
  int handleDIV(Line line);
  int handleNOT(Line line);
  int handleAND(Line line);
  int handleOR(Line line);
  int handleXOR(Line line);
  int handleSHL(Line line);
  int handleSHR(Line line);
  int handleLD(Line line);
  int handleST(Line line);
  int handleCSRRD(Line line);
  int handleCSRWR(Line line);

  bool isSymbol(string operand);
  Symbol findSymbolInSymbolTable(string operand);
  int handleJumps(Line line, char oc, char firstMod, char secondMod);
  int loadMEM_DIR(Line line);
  int storeMEM_DIR_INDIR(Line line, char mod);
  void updateCurrentSectionInList(BinaryLine newLine);


  int handleGLOBAL(Line line);
  int handleEXTERN(Line line);
  int handleSECTION(Line line);
  int handleWORD(Line line);
  int handleSKIP(Line line);
  int handleEND(Line line);
  int handleLABEL(Line line);

  LiteralPoolElem doesLiteralExistsInPool( vector<LiteralPoolElem> literalsPool, string literal);
  int fixInstruction(LiteralPoolElem elem, Section& section, BackpatchElem bp, int positionOfLiteral);
  int backpatch();

  void createBinary();
  void createText();
};
#endif