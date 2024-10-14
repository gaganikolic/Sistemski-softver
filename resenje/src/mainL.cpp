#include "../inc/linker.hpp"
#include <iostream>
#include <stdio.h>
#include <string>
#include <vector>

using namespace std;
 
int main(int argc, char* argv[]) {
  vector<string> files;
  string outputFile;
  bool hexFlag = false;
  map<string, int> places;

  for (int i = 1; i < argc; ++i) {
    string arg = argv[i];
    if (arg == "-hex") {
      hexFlag = true; 
    } else if (arg == "-o") {
      if (i + 1 < argc) {
        outputFile = argv[++i];  
      } else {
        cerr << "Nije prosledjen naziv izlaznog fajla LINKERA!" << endl;
        return -1;
      }
    } else if (arg.substr(0, 7) == "-place=") {
      string placeArg = arg.substr(7);
      int atPos = placeArg.find('@');
      if (atPos != string::npos) {
        string sectionName = placeArg.substr(0, atPos);
        string addressStr = placeArg.substr(atPos + 1);
        int address = stol(addressStr, nullptr, 16); // Pretvoriti iz heksadecimalnog u int
        places[sectionName] = address;
      } else {
        cerr << "Neispravan format za -place opciju: " << arg << endl;
        return -1;
      }
    } else {
      files.push_back(arg);
    }
  }

  Linker linker(outputFile);
  for(int i = 0; i < files.size(); i++) {
    linker.loadFile(files[i]);
  }
  linker.setPlacements(places);
  linker.handlePlace();
  int check12 = linker.createLinkerSymbolTable();
  linker.updateAddressesUsingRelocation();

  bool check3 = linker.sectionsOverlap();
  
  // Generisanje izlaznog fajla linkera
  if(check12 != -1 && !check3 && hexFlag) linker.createOutputFile();
  return 0;
}
