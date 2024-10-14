#include "../inc/linker.hpp"
#include <iostream>
#include <string>
#include <fstream>
#include <iomanip>
#include <algorithm>

bool Linker::sectionExistsInSections(string sectionName) {
  for(auto& elem: linkerSections) {
    if(elem.sectionName == sectionName) return true;
  }
  return false;
}

int Linker::addSectionTableFromInputFile(ifstream& file, string nameOfInputFile) {
  SectionTable table(nameOfInputFile);
  int sectionTableSize;
  file.read(reinterpret_cast<char*>(&sectionTableSize), sizeof(sectionTableSize));
  for (int i = 0; i < sectionTableSize; ++i) {
    // Naziv sekcije
    int sectionNameLen;
    file.read(reinterpret_cast<char*>(&sectionNameLen), sizeof(sectionNameLen));
    string sectionName(sectionNameLen, '\0');
    file.read(&sectionName[0], sectionNameLen);
    // Redni broj sekcije u fajlu
    int sectionStartAddress;
    file.read(reinterpret_cast<char*>(&sectionStartAddress), sizeof(sectionStartAddress));
    // Velicina sekcije
    int sectionSize;
    file.read(reinterpret_cast<char*>(&sectionSize), sizeof(sectionSize));
    
    table.sectionTableOfFile.push_back(Section(sectionName, sectionStartAddress, sectionSize));
  }
  sectionTables.push_back(table);
  return 1;
}

int Linker::addSymbolTableFromInputFile(ifstream& file, string nameOfInputFile) {
  SymbolTable table(nameOfInputFile);
  int numSymbols;
  file.read(reinterpret_cast<char*>(&numSymbols), sizeof(numSymbols));
  for (int i = 0; i < numSymbols; ++i) {  
    // Čitanje vrednosti simbola
    Symbol symbol;
    file.read(reinterpret_cast<char*>(&symbol.value), sizeof(symbol.value));
    file.read(reinterpret_cast<char*>(&symbol.size), sizeof(symbol.size));
    file.read(reinterpret_cast<char*>(&symbol.type), sizeof(symbol.type));
    file.read(reinterpret_cast<char*>(&symbol.bind), sizeof(symbol.bind));
    file.read(reinterpret_cast<char*>(&symbol.section), sizeof(symbol.section));
    size_t symbolNameSize;
    file.read(reinterpret_cast<char*>(&symbolNameSize), sizeof(symbolNameSize));
    symbol.symbol.resize(symbolNameSize);
    file.read(&symbol.symbol[0], symbolNameSize);
    // Dodavanje simbola u tabelu
    table.symbolTableOfFile.push_back(symbol);
  }
  symbolTables.push_back(table);  
  return 1;
}

int Linker::addSectionContentFromInputFile(ifstream& file, string nameOfInputFile) {
  // Učitavanje broja sekcija
  int numSections;
  file.read(reinterpret_cast<char*>(&numSections), sizeof(numSections));
    
  for(int i = 0; i < numSections; i++) {
    // Učitavanje veličine sekcije
    int sectionSize;
    file.read(reinterpret_cast<char*>(&sectionSize), sizeof(sectionSize));

    // Učitavanje naziva sekcije
    int sectionNameLen;
    file.read(reinterpret_cast<char*>(&sectionNameLen), sizeof(sectionNameLen));
    string sectionName(sectionNameLen, '\0');
    file.read(&sectionName[0], sectionNameLen);
        
    // Učitavanje sadržaja sekcije
    vector<char> sectionContent(sectionSize);
    file.read(sectionContent.data(), sectionSize);

    // Velicina relokacione tabele sekcije
    vector<RelocationElem> relTab;
    int relSize;
    file.read(reinterpret_cast<char*>(&relSize), sizeof(relSize));
    for (int i = 0; i < relSize; ++i) {
      int offset;
      file.read(reinterpret_cast<char*>(&offset), sizeof(offset));
      int symbolNameLen;
      file.read(reinterpret_cast<char*>(&symbolNameLen), sizeof(symbolNameLen));
      string symbolName(symbolNameLen, '\0');
      file.read(&symbolName[0], symbolNameLen);
      int addend;
      file.read(reinterpret_cast<char*>(&addend), sizeof(addend));

      relTab.push_back(RelocationElem(offset, symbolName, addend));
    }
    
    SectionData sd;
    sd.sectionSize = sectionSize;
    sd.sectionName = sectionName;
    sd.binaryLines = sectionContent;
    sd.relocationTableOfSection = relTab;

    //Dodavanje procitane sekcije u mapu
    connectSections(sd, nameOfInputFile);
  }
  return 1;
}

int Linker::loadFile(const string& filePath) {
  ifstream file(filePath, ios::binary | ios::in);
  if (!file) {
    cout << "Nije moguce otvoriti fajl: " << filePath << endl;
    return -1;
  }

  // Ime fajla
  size_t lastSlash = filePath.find_last_of("/\\");
  string fileNameWithExtension = filePath.substr(lastSlash + 1);
  size_t dotPos = fileNameWithExtension.find_last_of('.');
  string nameOfInputFile = fileNameWithExtension.substr(0, dotPos);

  addSectionTableFromInputFile(file, nameOfInputFile);
  addSymbolTableFromInputFile(file, nameOfInputFile);
  addSectionContentFromInputFile(file, nameOfInputFile);

  file.close();
  return 1;
}

int Linker::updateSymbolTable(string sectionName, string fileName, int increment) {
  int index = -1;
  for(int i = 0; i < symbolTables.size(); i++) {
    if(symbolTables[i].file == fileName) {
      index = i;
      break;
    }
  }

  vector<Symbol>&  symTabToUpdate = symbolTables[index].symbolTableOfFile;
  // Odredjujemo koja sekcija se spojila
  int section = -1;
  for(auto& symbol: symTabToUpdate) {
    if(symbol.symbol == sectionName && symbol.type == SymbolType::SCTN) {
      section = symbol.section;
      break;
    }
  }
  if(section == -1) {
    cout << "Sekcija sa ovim imenom ne postoji u tabeli simbola ulaznog fajla " << fileName << endl;
    return -1;
  }
  // Simbole koji pripadaju spojenoj sekciji sa nekom drugom, na VALUE dodajemo velicinu sekcije kojoj se data pridruzila
  for(auto& symbol: symTabToUpdate) {
    if(symbol.section == section && symbol.type == SymbolType::NOTYPE) {
      symbol.value += increment;
    }
  }
  return 1;
}

int Linker::connectSections(SectionData& section, string fileName) {
  if(sectionExistsInSections(section.sectionName)){
    SectionData postojecaSekcija;
    for(auto& elem: linkerSections) {
      if(elem.sectionName == section.sectionName) {
        postojecaSekcija = elem;
        break;
      }
    }
    for(int i = 0; i < section.binaryLines.size(); i++) {
      // Upisivanje sadrzaja nove sekcije i njenog bazena literala na kraj postojece sekcije
      postojecaSekcija.binaryLines.push_back(section.binaryLines.at(i));
    }
    
    // Azuriranje tabele simbola fajla kome pripada sekcija SECTION (azuriranje VALUE)
    int update = updateSymbolTable(section.sectionName, fileName, postojecaSekcija.sectionSize);
    if(update == -1) return -1;

    // Azuriranje tabele relokacija sekcije (Na offset i addend se dodaje postojecaSekcija.size)
    // I spajanje tabela relokacija
    for(auto& relElem: section.relocationTableOfSection){
      relElem.offset += postojecaSekcija.sectionSize;
      //relElem.addend += postojecaSekcija.sectionSize;
      RelocationElem newElem = relElem;
      postojecaSekcija.relocationTableOfSection.push_back(newElem);
    }

    // Azuriranje velicine sekcije
    postojecaSekcija.sectionSize += section.sectionSize;

    for(auto& sec: linkerSections) {
      if(sec.sectionName == postojecaSekcija.sectionName) {
        sec = postojecaSekcija;
      }
    }

  } else {
    linkerSections.push_back(section);
  }
 return 1;
}

Linker::Symbol& Linker::findSymbolInLinkerSymbolTable(string symbolName) {
  for(auto& symbol: linkerSymbolTable) {
    if(symbol.symbol == symbolName) {
      return symbol;
    }
  }
  return defaultSymbol;
}

int Linker::fixSymbolValue() {
  for(auto& symbol: linkerSymbolTable) {
    if(symbol.section == -1) {
      cout << "Simbol " << symbol.symbol << " nije definisan. Nije moguce izvrsiti linkovanje!" << endl;
      return -1;
    }
    int startAddressOfSection = -1;
    for(auto& sec: linkerSymbolTable) {
      if(sec.section == symbol.section && sec.type == SymbolType::SCTN){
        startAddressOfSection = sec.value;
        break;
      }
    }
  
    if(symbol.type != SymbolType::SCTN){
      symbol.value += startAddressOfSection; 
    }
  }
  return 1;
}

int Linker::handlePlace() {
  int start = 0;
  for(auto& place: placements) {
    string name = place.first;
    unsigned int startAddress = static_cast<unsigned int>(place.second);

    for(auto& sec: linkerSections) {
      if(sec.sectionName == name) {
        sec.startAddress = startAddress;
        if(startAddress > start) {
          start = startAddress + sec.sectionSize;
        }
        break;
      }
    }
  }

  // Postavljanje pocetne adrese ostalim sekcijama
  for (auto& sec : linkerSections) {
    if (sec.startAddress == -1) {
      sec.startAddress = start;
      start += sec.sectionSize;
    }
  }
  return 1;
}

int Linker::findNdx(int i, int ndx) {
  auto& sectionTable = sectionTables[i];  //Ako obradjujem prvu tabelu simbola iz symbolTables, to je i prva tabela sekcija iz sectionTables
  string sectionName = sectionTable.sectionTableOfFile[ndx].sectionName;
  for(int i = 0; i < linkerSections.size(); i++) {
    if(linkerSections[i].sectionName == sectionName) {
      return i;
    }
  }
  return -1;
}

int Linker::combineSymbolTables() {
  for(int i = 0; i < symbolTables.size(); i++) {
    auto& symbolTable = symbolTables[i]; 
    for(auto& symbol: symbolTable.symbolTableOfFile) {
      if(symbol.type == SymbolType::SCTN) continue;
      if(symbol.bind == Binding::LOCAL_SYM) continue;

      Symbol& linkingSymbol = findSymbolInLinkerSymbolTable(symbol.symbol);

      if(linkingSymbol.symbol != "") {
        //Simbol postoji u tabeli linkera
        if(symbol.section == -1) {
          continue;
        }

        if(linkingSymbol.section != -1) {
          cout << "Visestruka definicija simbola " << linkingSymbol.symbol << endl;
          return -1;
        }

        int ndx = findNdx(i, symbol.section);
        linkingSymbol.section = ndx;
        linkingSymbol.value = symbol.value;
      
      } else {
        //Simbol ne postoji u tabeli linkera
        if(symbol.section != -1) {
          //simbol je definisan
          int ndx = findNdx(i, symbol.section);
          symbol.section = ndx;
        }
        linkerSymbolTable.push_back(symbol);
      }
    }
  }
  return 1;
}

int Linker::createLinkerSymbolTable() {
  map<string, int> sectionNameToIndex; 

  // Kreiranje mape sekcija
  for (int i = 0; i < linkerSections.size(); i++) {
    // Dodavanje SEKCIJE u tabelu simbola
    Symbol sym;
    sym.symbol = linkerSections[i].sectionName;
    sym.section = i;
    sym.bind = Binding::LOCAL_SYM;
    sym.size = 0;
    sym.type = SymbolType::SCTN;
    sym.value = linkerSections[i].startAddress;

    linkerSymbolTable.push_back(sym);
  }
  
  int res1 = combineSymbolTables();
  int res2 = fixSymbolValue();
 
  /*cout << setw(15) << "Name" << setw (8) << "Value" << setw(8) << "Ndx" << setw(8) << "Bind" << setw(8) << "Type" << endl;
  for(auto& sym: linkerSymbolTable) {
    cout << setw(15) << hex << sym.symbol << " " << setw(8) <<  sym.value << " " << setw(8) <<  sym.section;
    cout <<  setw(8);
    if(sym.bind == Binding::LOCAL_SYM) cout << "LOC";
    else cout << "GLOB";
    cout << setw(8);
    if(sym.type == SymbolType::NOTYPE) cout << "NOTYP";
    else cout << "SCTN";
    cout << endl;
  }*/
  if(res1 == -1 || res2 == -1) {
    //cout << "Ne treba izvrsiti linkovanje" << endl;
    return -1;
  }
  return 1;
}

bool Linker::sectionsOverlap() {
  struct SectionInfo {
    string name;
    int size;
    int start;
    SectionInfo(string name, int size, int start): name(name), start(start), size(size) {}
  };
  vector<SectionInfo> pomSections;
  for(int i = 0; i < linkerSections.size(); i++) {
    SectionInfo s(linkerSections[i].sectionName, linkerSections[i].sectionSize, linkerSections[i].startAddress);
    pomSections.push_back(s);
  }
  std::sort(pomSections.begin(), pomSections.end(), [](const SectionInfo& a, const SectionInfo& b) {
    return a.start < b.start;
  });

  for(int i = 0; i < pomSections.size() - 1; i++) {
    if(pomSections[i].start + pomSections[i].size > pomSections[i + 1].start) {
      cout << "Sekcije " << pomSections[i].name << " i " << pomSections[i + 1].name << " se preklapaju!" << endl; 
      return true;
    }
  }

  return false;    
}

int Linker::updateAddressesUsingRelocation() {
  for(auto& section: linkerSections) {
    for(auto& relElem: section.relocationTableOfSection){
      Symbol sym = findSymbolInLinkerSymbolTable(relElem.symbolName);
      int fix = sym.value + relElem.addend;
      section.binaryLines[relElem.offset] = (fix >> 24) & 0xFF;
      section.binaryLines[relElem.offset + 1] = (fix >> 16) & 0xFF;
      section.binaryLines[relElem.offset + 2] = (fix >> 8) & 0xFF;
      section.binaryLines[relElem.offset + 3] = fix & 0xFF;

    }
  }
  return 1;
}

int Linker::createOutputFile() {
  ofstream file(outputFileName, ios::out);
  if (!file) {
    cout << "Nije moguce otvoriti faj " << outputFileName << " za ispis" << endl;
    return -1;
  }

  int linkerSectionsSize = linkerSections.size();
  file.write(reinterpret_cast<const char*>(&linkerSectionsSize), sizeof(linkerSectionsSize));  

  for (const auto& section : linkerSections) {
    int address = section.startAddress;
    int sectionSize = section.sectionSize;
    
    file.write(reinterpret_cast<const char*>(&address), sizeof(address));
    file.write(reinterpret_cast<const char*>(&sectionSize), sizeof(sectionSize));

    // Ispis sadrzaja sekcije
    for(int i = 0; i < section.binaryLines.size(); ){
      char a = section.binaryLines[i];
      char b = section.binaryLines[i + 1];
      char c = section.binaryLines[i + 2];
      char d = section.binaryLines[i + 3];
      i += 4;

      file.write(reinterpret_cast<const char*>(&d), sizeof(d));
      file.write(reinterpret_cast<const char*>(&c), sizeof(c));
      file.write(reinterpret_cast<const char*>(&b), sizeof(b));
      file.write(reinterpret_cast<const char*>(&a), sizeof(a));

    }
  }

  // Ispis sekcija
  /*for (const auto& section : linkerSections) {
    int address = section.startAddress;
    int sectionSize = section.sectionSize;
    
    file.write(reinterpret_cast<const char*>(&address), sizeof(address));
    file.write(reinterpret_cast<const char*>(&sectionSize), sizeof(sectionSize));

    for(auto& byte: section.binaryLines) {
      file.write(reinterpret_cast<const char*>(&byte), sizeof(byte));
    } 
  }*/
  file.close();
  return 1;
}




