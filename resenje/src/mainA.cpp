#include <iostream>
#include <stdio.h>
#include <string>

extern int yyparse();
extern FILE* yyin;
const char* nameOfInputFile;

using namespace std;
 
int main(int argc, char* argv[]) {
    

    yyin = fopen(argv[3], "r"); // Open input file
    nameOfInputFile = argv[2];

    if (!yyin) {
        cout << "INPUT FILE NOT RECOGNIZED" << endl;
        return 1;
    }

    yyparse(); // Start parsing

    fclose(yyin); // Close file
    return 0;
}
