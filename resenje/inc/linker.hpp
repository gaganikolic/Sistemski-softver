#ifndef _LINKER_HPP
#define _LINKER_HPP

#include <vector>
#include <string>
#include <ostream>
#include <iostream>
#include <map>

using namespace std;

class Linker {
private:
  enum Binding {LOCAL_SYM, GLOBAL_SYM, ERR_BIND};

  enum SymbolType {SCTN, NOTYPE, ERR_TYPE};

  struct Section {
    string sectionName; //Ime sekcije
    int startAddress;    //Pocetna adresa sekcije
    int sectionSize;    //Velicina sekcije
    Section(string sectionName, int startAddress, int sectionSize) {
      this->sectionName = sectionName;
      this->startAddress = startAddress;
      this->sectionSize = sectionSize;
    }
  };
  struct SectionTable {
    string file;        //Ime fajla u kojem se nalazi sekcija
    vector<Section> sectionTableOfFile;
    SectionTable(string file) {
      this->file = file;
    }
  };

  struct Symbol {
    int value;          //vrednost simbola(adresa na kojoj se taj simbol nalazi)
    int size;     
    SymbolType type;    //SCTN(kada je simbol naziv sekcije), NOTYP
    Binding bind;       //LOCAL,GLOBAL
    int section;        //kojoj sekciji simbol pripada (ndx)
    string symbol;      //naziv simbola
    Symbol() {
      this->value = -1;
      this->size = -1;
      this->type = ERR_TYPE;
      this->bind = ERR_BIND;
      this->section = -1;
      this->symbol = "";
    }
    Symbol(int value, int size, SymbolType type, Binding bind, int section, string symbol) {
      this->value = value;
      this->size = size;
      this->type = type;
      this->bind = bind;
      this->section = section;
      this->symbol = symbol;
    }
  };
  struct SymbolTable {
    string file;
    vector<Symbol> symbolTableOfFile;
    SymbolTable(string file) {
      this->file = file;
    }
  };

  struct RelocationElem {
    int offset;
    string symbolName;
    int addend;
    RelocationElem(int offset, string symbolName, int addend) {
      this->offset = offset;
      this->symbolName = symbolName;
      this->addend = addend;
    }
  };
 
  struct SectionData {
    int startAddress = -1;
    int sectionSize = -1;
    string sectionName = "";
    vector<char> binaryLines;
    vector<RelocationElem> relocationTableOfSection;
    SectionData() {}
  };

  vector<SectionTable> sectionTables; 
  vector<SymbolTable> symbolTables; 
  vector<SectionData> linkerSections;
  vector<Symbol> linkerSymbolTable;

  int address = 0;
  string outputFileName;
  map<string, int> placements;
  Symbol defaultSymbol;

public:
  Linker(string outputFileName) {
    this->outputFileName = outputFileName;
  }

  void setPlacements(map<string, int> p) {
    this->placements = p;
  }

  int addSectionTableFromInputFile(ifstream& file, string nameOfInputFile);
  int addSymbolTableFromInputFile(ifstream& file, string nameOfInputFile);
  int addSectionContentFromInputFile(ifstream& file, string nameOfInputFile);
  int loadFile(const string& filePath);

  int connectSections(SectionData& sd, string fileName);
  bool sectionExistsInSections(string sectionName);
  int updateSymbolTable(string sectionName, string fileName, int increment);
  Symbol& findSymbolInLinkerSymbolTable(string symbolName);
  int fixSymbolValue();
  int createLinkerSymbolTable();
  int updateAddressesUsingRelocation();
  int combineSymbolTables();
  int findNdx(int i, int ndx);

  bool sectionsOverlap();
  int handlePlace();

  int createOutputFile();
};
#endif