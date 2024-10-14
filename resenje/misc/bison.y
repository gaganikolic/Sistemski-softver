%{
  #include <iostream>
  #include <cstdio>
  #include <vector>
  #include <string>
  #include "./inc/assembler.hpp"

  /* Ono sto BISON ocekuje od FLEX */
  extern int linenumber; 
  extern int yylex();   

  void yyerror(const char* e); 

  extern const char* nameOfInputFile;
  Assembler* assembler = new Assembler(nameOfInputFile);

  char directive;
  vector<string> symbol_list;
 
%}

/* Tipovi tokena koje FLEX vraca */
%union {
  int intVal;
  char* strVal;
}

/* Tokeni */
%token HALT INT IRET CALL RET JMP BEQ BNE BGT PUSH POP XCHG ADD SUB MUL DIV NOT AND OR XOR SHL SHR LD ST CSRRD CSRWR
%token COLON SEMI DOLLAR PERCENT COMMA LSQUARE RSQUARE PLUS MINUS
%token GLOBAL EXTERN SECTION WORD SKIP ASCII EQU END
%token<intVal> GP_REGISTER CS_REGISTER
%token END_LINE COMMENT ERROR
%token<strVal> LITERAL SYMBOL LABEL STRING

%type<strVal> OPERAND

%%

PROGRAM_LINE:
            |
            PROGRAM_LINE OPERATION
            |
            PROGRAM_LINE DIRECTIVE
            |
            PROGRAM_LINE DIRECTIVE OPERAND
            { symbol_list.push_back($3); }
            |
            PROGRAM_LINE ENDL
            |
            PROGRAM_LINE COMMENTS
            |
            PROGRAM_LINE LABELS
            |
            PROGRAM_LINE COMMA OPERAND
            { symbol_list.push_back($3); }

OPERAND: SYMBOL
         |
         LITERAL

OPERATION: HALT 
          { 
            Assembler::Line newLine(Assembler::AddressingMode::NO_ADDRESSING_MODE, Assembler::Instruction::HALT, -1, -1, -1, ""); 
            assembler->handleHALT(newLine);
          }
          | 
          INT 
          { 
            Assembler::Line newLine(Assembler::AddressingMode::NO_ADDRESSING_MODE, Assembler::Instruction::INT, -1, -1, -1, "");
            assembler->handleINT(newLine);
          }
          | 
          IRET 
          { 
            Assembler::Line newLine(Assembler::AddressingMode::NO_ADDRESSING_MODE, Assembler::Instruction::IRET, -1, -1, -1, ""); 
            assembler->handleIRET(newLine);
          }
          | 
          CALL OPERAND
          { 
            Assembler::Line newLine(Assembler::AddressingMode::NO_ADDRESSING_MODE, Assembler::Instruction::CALL, -1, -1, -1, $2);
            int result = assembler->handleCALL(newLine);
            if(result == -1) printf("GRESKA: Instrukcija CALL");
          }
          | 
          RET 
          { 
            Assembler::Line newLine(Assembler::AddressingMode::NO_ADDRESSING_MODE, Assembler::Instruction::RET, -1, -1, -1, "");
            assembler->handleRET(newLine);
          }
          | 
          JMP OPERAND
          { 
            Assembler::Line newLine(Assembler::AddressingMode::NO_ADDRESSING_MODE, Assembler::Instruction::JMP, 0, 0, 0, $2);
            int result = assembler->handleJMP(newLine);
            if(result == -1) printf("GRESKA: Instrukcija JMP");
          }
          | 
          BEQ GP_REGISTER COMMA GP_REGISTER COMMA OPERAND
          { 
            Assembler::Line newLine(Assembler::AddressingMode::NO_ADDRESSING_MODE, Assembler::Instruction::BEQ, $2, $4, -1, $6); 
            int result = assembler->handleBEQ(newLine);
            if(result == -1) printf("GRESKA: Instrukcija BEQ");
          }
          | 
          BNE GP_REGISTER COMMA GP_REGISTER COMMA OPERAND
          { 
            Assembler::Line newLine(Assembler::AddressingMode::NO_ADDRESSING_MODE, Assembler::Instruction::BNE, $2, $4, -1, $6);
            int result = assembler->handleBNE(newLine);
            if(result == -1) printf("GRESKA: Instrukcija BNE");
          }
          | 
          BGT GP_REGISTER COMMA GP_REGISTER COMMA OPERAND
          { 
            Assembler::Line newLine(Assembler::AddressingMode::NO_ADDRESSING_MODE, Assembler::Instruction::BGT, $2, $4, -1, $6); 
            int result = assembler->handleBGT(newLine);
            if(result == -1) printf("GRESKA: Instrukcija BGT");
          } 
          | 
          PUSH GP_REGISTER 
          { 
            Assembler::Line newLine(Assembler::AddressingMode::NO_ADDRESSING_MODE, Assembler::Instruction::PUSH, $2, -1, -1, "");
            assembler->handlePUSH(newLine);
          }
          | 
          POP GP_REGISTER
          { 
            Assembler::Line newLine(Assembler::AddressingMode::NO_ADDRESSING_MODE, Assembler::Instruction::POP, $2, -1, -1, ""); 
            assembler->handlePOP(newLine);
          }
          | 
          XCHG GP_REGISTER COMMA GP_REGISTER
          { 
            Assembler::Line newLine(Assembler::AddressingMode::NO_ADDRESSING_MODE, Assembler::Instruction::XCHG, -1, $2, $4, "");
            assembler->handleXCHG(newLine);
          }
          | 
          ADD GP_REGISTER COMMA GP_REGISTER
          { 
            Assembler::Line newLine(Assembler::AddressingMode::NO_ADDRESSING_MODE, Assembler::Instruction::ADD, $4, $4, $2, "");
            assembler->handleADD(newLine);
           }
          | 
          SUB GP_REGISTER COMMA GP_REGISTER
          { 
            Assembler::Line newLine(Assembler::AddressingMode::NO_ADDRESSING_MODE, Assembler::Instruction::SUB, $4, $4, $2, "");
            assembler->handleSUB(newLine);
          }
          | 
          MUL GP_REGISTER COMMA GP_REGISTER
          { 
            Assembler::Line newLine(Assembler::AddressingMode::NO_ADDRESSING_MODE, Assembler::Instruction::MUL, $4, $4, $2, "");
            assembler->handleMUL(newLine);
          }
          | 
          DIV GP_REGISTER COMMA GP_REGISTER
          { 
            Assembler::Line newLine(Assembler::AddressingMode::NO_ADDRESSING_MODE, Assembler::Instruction::DIV, $4, $4, $2, "");
            assembler->handleDIV(newLine);
          }
          | 
          NOT GP_REGISTER 
          { 
            Assembler::Line newLine(Assembler::AddressingMode::NO_ADDRESSING_MODE, Assembler::Instruction::NOT, $2, $2, -1, ""); 
            assembler->handleNOT(newLine);
          }
          | 
          AND GP_REGISTER COMMA GP_REGISTER
          { 
            Assembler::Line newLine(Assembler::AddressingMode::NO_ADDRESSING_MODE, Assembler::Instruction::AND, $4, $4, $2, "");
            assembler->handleAND(newLine);
          }
          | 
          OR GP_REGISTER COMMA GP_REGISTER
          { 
            Assembler::Line newLine(Assembler::AddressingMode::NO_ADDRESSING_MODE, Assembler::Instruction::OR, $4, $4, $2, "");
            assembler->handleOR(newLine);
          }
          | 
          XOR GP_REGISTER COMMA GP_REGISTER
          { 
            Assembler::Line newLine(Assembler::AddressingMode::NO_ADDRESSING_MODE, Assembler::Instruction::XOR, $4, $4, $2, ""); 
            assembler->handleXOR(newLine);
          }
          | 
          SHL GP_REGISTER COMMA GP_REGISTER
          { 
            Assembler::Line newLine(Assembler::AddressingMode::NO_ADDRESSING_MODE, Assembler::Instruction::SHL, $4, $4, $2, "");
            assembler->handleSHL(newLine);
          }
          | 
          SHR GP_REGISTER COMMA GP_REGISTER
          { 
            Assembler::Line newLine(Assembler::AddressingMode::NO_ADDRESSING_MODE, Assembler::Instruction::SHR, $4, $4, $2, "");
            assembler->handleSHR(newLine);
          }
          | 
          LD DOLLAR OPERAND COMMA GP_REGISTER 
          { 
            Assembler::Line newLine(Assembler::AddressingMode::MEM_DIR, Assembler::Instruction::LD, $5, -1, -1, $3); 
            int result = assembler->handleLD(newLine);
            if(result == -1) printf("GRESKA: Instrukcija LD");
          }
          |
          LD OPERAND COMMA GP_REGISTER 
          { 
            Assembler::Line newLine(Assembler::AddressingMode::MEM_INDIR, Assembler::Instruction::LD, $4, -1, -1, $2); 
            int result = assembler->handleLD(newLine);
            if(result == -1) printf("GRESKA: Instrukcija LD");
          }
          |
          LD GP_REGISTER COMMA GP_REGISTER 
          { 
            Assembler::Line newLine(Assembler::AddressingMode::REG_DIR, Assembler::Instruction::LD, $4, $2, -1, "");
            int result = assembler->handleLD(newLine);
            if(result == -1) printf("GRESKA: Instrukcija LD");
          }
          |
          LD LSQUARE GP_REGISTER RSQUARE COMMA GP_REGISTER
          { 
            Assembler::Line newLine(Assembler::AddressingMode::REG_INDIR, Assembler::Instruction::LD, $6, $3, -1, ""); 
            int result = assembler->handleLD(newLine);
            if(result == -1) printf("GRESKA: Instrukcija LD");
          }
          |
          LD LSQUARE GP_REGISTER PLUS OPERAND RSQUARE COMMA GP_REGISTER
          { 
            Assembler::Line newLine(Assembler::AddressingMode::REG_INDIR_POM, Assembler::Instruction::LD, $8, $3, -1, $5);
            int result = assembler->handleLD(newLine);
            if(result == -1) printf("GRESKA: Instrukcija LD");
          }
          |
          ST GP_REGISTER COMMA GP_REGISTER
          { 
            Assembler::Line newLine(Assembler::AddressingMode::REG_DIR, Assembler::Instruction::ST, $4, $2, -1, ""); 
            int result = assembler->handleST(newLine);
            if(result == -1) printf("GRESKA: Instrukcija ST");
          }
          |
          ST GP_REGISTER COMMA DOLLAR OPERAND
          { 
            Assembler::Line newLine(Assembler::AddressingMode::MEM_DIR, Assembler::Instruction::ST, $2, -1, -1, $5); 
            int result = assembler->handleST(newLine);
            if(result == -1) printf("GRESKA: Instrukcija ST");
          }
          |
          ST GP_REGISTER COMMA OPERAND
          { 
            Assembler::Line newLine(Assembler::AddressingMode::MEM_INDIR, Assembler::Instruction::ST, $2, -1, -1, $4);
            int result = assembler->handleST(newLine);
            if(result == -1) printf("GRESKA: Instrukcija ST");
          } 
          |
          ST GP_REGISTER COMMA LSQUARE GP_REGISTER RSQUARE
          { 
            Assembler::Line newLine(Assembler::AddressingMode::REG_INDIR, Assembler::Instruction::ST, $5, $2, -1, ""); 
            int result = assembler->handleST(newLine);
            if(result == -1) printf("GRESKA: Instrukcija ST");
          }
          |
          ST GP_REGISTER COMMA LSQUARE GP_REGISTER PLUS OPERAND RSQUARE
          { 
            Assembler::Line newLine(Assembler::AddressingMode::REG_INDIR_POM, Assembler::Instruction::ST, $5, $2, -1, $7); 
            int result = assembler->handleST(newLine);
            if(result == -1) printf("GRESKA: Instrukcija ST");
          }
          |
          CSRRD CS_REGISTER COMMA GP_REGISTER
          { 
            Assembler::Line newLine(Assembler::AddressingMode::NO_ADDRESSING_MODE, Assembler::Instruction::CSRRD, $4, $2, -1, "");
            assembler->handleCSRRD(newLine);
          }
          | 
          CSRWR GP_REGISTER COMMA CS_REGISTER
          { 
            Assembler::Line newLine(Assembler::AddressingMode::NO_ADDRESSING_MODE, Assembler::Instruction::CSRWR, $4, $2, -1, ""); 
            assembler->handleCSRWR(newLine);
          }

DIRECTIVE: GLOBAL 
           { directive = 'g'; }
           | 
           EXTERN 
           { directive = 'e'; }
           | 
           SECTION SYMBOL 
           { 
            Assembler::Line newLine(Assembler::Directive::SECTION, $2, symbol_list);
            int result = assembler->handleSECTION(newLine);
            if(result == -1) printf("GRESKA: Direktiva SECTION");
           }
           | 
           WORD 
           { directive = 'w'; }
           | 
           SKIP LITERAL 
           { 
            Assembler::Line newLine(Assembler::Directive::SKIP, $2, symbol_list); 
            int result = assembler->handleSKIP(newLine);
            if(result == -1) printf("GRESKA: Direktiva SKIP");
           }
           | 
           ASCII STRING
           | 
           END 
           { 
            Assembler::Line newLine(Assembler::Directive::END, "", symbol_list);
            assembler->handleEND(newLine);

            assembler->nameOfFile = nameOfInputFile;
            
            int success = assembler->backpatch();
            if(success != -1) {
              assembler->createBinary();
              assembler->createText();            
            } else {
              printf("GRESKA: Backpatch");
            }
           }

LABELS: LABEL
        { 
          Assembler::Line newLine(Assembler::Directive::LABEL, $1, symbol_list); 
          int result = assembler->handleLABEL(newLine);
          if(result == -1) {
            printf("GRESKA: Obrada LABELE");
          } 
        }

ENDL: END_LINE
      {
        if(directive == 'g'){
          Assembler::Line newLine(Assembler::Directive::GLOBAL, "", symbol_list); 
          int result = assembler->handleGLOBAL(newLine);
          if(result == -1) {
            printf("GRESKA: Direktiva GLOBAL");
          }

          directive = '\0';
          symbol_list.clear();
        } else if(directive == 'e'){
          Assembler::Line newLine(Assembler::Directive::EXTERN, "", symbol_list); 
          int result = assembler->handleEXTERN(newLine);
          if(result == -1) {
            printf("GRESKA: Direktiva EXTERN");
          }
          
          directive = '\0';
          symbol_list.clear();
        } else if(directive == 'w'){
          Assembler::Line newLine(Assembler::Directive::WORD, "", symbol_list); 
          int result = assembler->handleWORD(newLine);
          if(result == -1) {
            printf("GRESKA: Direktiva WORD");
          } 
          
          directive = '\0';
          symbol_list.clear();
        }
      }

COMMENTS: COMMENT

%%

void yyerror(const char* e){
  printf("\n---------------------------------\nERROR ON LINE: %d\n", linenumber + 1);
  exit(-1);
}