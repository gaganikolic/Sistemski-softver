all: flexBison compileAssembler compileLinker compileEmulator clean

flexBison:
	flex -o lex.yy.cpp "./misc/flex.l"
	bison -d -o bison.tab.cpp "./misc/bison.y"

compileAssembler:
	g++ -g ./src/mainA.cpp lex.yy.cpp bison.tab.cpp ./src/assembler.cpp -o assembler

compileLinker:
	g++ -g ./src/mainL.cpp ./src/linker.cpp -o linker

compileEmulator:
	g++ -g ./src/mainE.cpp ./src/emulator.cpp -o emulator

clean:
	rm -f lex.yy.cpp bison.tab.cpp bison.tab.hpp 
