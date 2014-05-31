#include "kind.h"
#include "lexer.h"
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <map>
// Use only the neeeded aspects of each namespace
using std::string;
using std::map;
using std::vector;
using std::endl;
using std::cerr;
using std::cout;
using std::cin;
using std::getline;
using std::exception;
using ASM::Token;
using ASM::Lexer;

class ASMException : public std::exception
{
private:
    string message;
public:
    ASMException(string message): message(message) {}
    virtual const char *what() const throw() { return message.c_str(); }
    ~ASMException() throw() {}
};

void printMipsInstruction(int instr) {
    char c = instr >> 24; // first 8 bits
    cout << c;
    c = instr >> 16; // first 8 bits
    cout << c;
    c = instr >> 8; // first 8 bits
    cout << c;
    c = instr; // first 8 bits
    cout << c;
}

void populateLabelMap(map<string, int> &labelMap, vector< vector<Token*> > &tokLines) {
    int lineNum = 0;
    
    vector<vector<Token*> >::iterator it;
    // through lines
    for(it = tokLines.begin(); it != tokLines.end(); )
    {
      vector<Token*>::iterator it2;
      bool nullLine = true;
      //through tokens in line, need to check non null
      for(it2 = it->begin(); it2 != it->end();/* ++it2*/)
      {
          if ( (*it2)->getKind() != ASM::LABEL) {
              nullLine = false;
              it2 ++;
              
          } else {
              //is a label

              string label = (*it2)->getLexeme(); //remove trailing colon
              label = label.substr(0, label.length()-1);

              //something before it is NOT a label
              if (!nullLine) {
                  throw string("ERROR: incorrect label location for label: " + label);
              }

              //Labels can only be defined once
              if (labelMap.find(label) == labelMap.end()) {
                  labelMap[label] = lineNum;
              } else {
                  throw string("ERROR: Duplicate label definition: "+ label);
              }

              //erase the label, and get next element
              it2 = (*it).erase(it2);
          }
      }

      if (it->size() == 0) {
          it = tokLines.erase(it);
      } else {
          ++ it;
      }

      if (!nullLine)
          lineNum += 4;
    }
}

// 3 params : add, sub, slt, sltu 
void verifyRTypeInstruction(vector<vector<Token*> >::iterator &it,
                            vector<Token*>::iterator &it2)
{
  if (it->end() - it2 != 6)
      throw string("ERROR! R type instruction has 3 arguments");

  // verify the next 3 params are registers
  if (!( (*(it2+1))->getKind() == ASM::REGISTER
         && (*(it2+2))->getKind() == ASM::COMMA
         && (*(it2+3))->getKind() == ASM::REGISTER
         && (*(it2+4))->getKind() == ASM::COMMA
         && (*(it2+5))->getKind() == ASM::REGISTER
          ) )
      throw string("ERROR! parameters must be registers");
}


// 2 params : mult, multu, div, and divu
void verifyRTypeInstructionTwoParams(vector<vector<Token*> >::iterator &it,
                            vector<Token*>::iterator &it2)
{
  if (it->end() - it2 != 4)
      throw string("ERROR! R type instruction has 3 arguments");

  // verify the next 3 params are registers
  if (!( (*(it2+1))->getKind() == ASM::REGISTER
         && (*(it2+2))->getKind() == ASM::COMMA
         && (*(it2+3))->getKind() == ASM::REGISTER
          ) )
      throw string("ERROR! parameters must be registers");
}

// jr, jalr + mfhi, mflo, lis
void verifyJTypeInstruction(vector<vector<Token*> >::iterator &it,
  vector<Token*>::iterator &it2) {
  if (it->end() - it2 != 2)
      throw string("ERROR! Invalid number of arguments");

  if ((*(it2+1))->getKind() != ASM::REGISTER)
      throw string("ERROR! Must give register as param");
}

// beq, bne
void verifyITypeInstruction(vector<vector<Token*> >::iterator &it,
  vector<Token*>::iterator &it2) {
  if (it->end() - it2 != 6)
      throw string("ERROR! Invalid number of arguments");

  // verify the next 3 params are registers
  if (!( (*(it2+1))->getKind() == ASM::REGISTER
         && (*(it2+2))->getKind() == ASM::COMMA
         && (*(it2+3))->getKind() == ASM::REGISTER
         && (*(it2+4))->getKind() == ASM::COMMA
         && ( (*(it2+5))->getKind() == ASM::INT  
             || (*(it2+5))->getKind() == ASM::HEXINT
             || (*(it2+5))->getKind() == ASM::ID )
          ) )
      throw string("ERROR! Invalid parameter types");
}

//gets the label position or throws invalid label exception
int getLabelPositionFromMap (map<string, int> &labelMap, const string &value) {
    std::map<string,int>::iterator mapit;
    mapit = labelMap.find( value );

    if (mapit != labelMap.end()) {
        int wordValue = mapit->second;
        return wordValue;
    }
        
    throw string("ERROR! Invalid Label");
}

int getBranchOffset (vector<Token*>::iterator &tokenIt, map<string, int> &labelMap, int lineNum) {
    int bValue = -1;
    if ( (*(tokenIt+5))->getKind() == ASM::ID )
    {
        bValue = getLabelPositionFromMap(labelMap, (*(tokenIt+5))->getLexeme());
        bValue = (bValue - lineNum -4)/4;

        if (bValue < -32768 || bValue > 32767)
            throw string("ERROR! Offset must be in range[-32768, 32767]");
    } 
    else
    {
        bValue = (*(tokenIt+5))->toInt();
        if ( 
            ((*(tokenIt+5))->getKind()==ASM::INT && (bValue < -32768 || bValue > 32767)) ||
            ((*(tokenIt+5))->getKind()==ASM::HEXINT && (bValue > 0xffff || bValue < 0))
            )
            throw string("ERROR! Offset must be in range[-32768, 32767]");
    }
    return bValue;
}

int main(int argc, char* argv[]){
  // Nested vector representing lines of Tokens
  // Needs to be used here to cleanup in the case
  // of an exception
  vector< vector<Token*> > tokLines;
  try{
    // Create a MIPS recognizer to tokenize
    // the input lines
    Lexer lexer;
    // Tokenize each line of the input
    string line;
    while(getline(cin,line)){
      tokLines.push_back(lexer.scan(line));
    }

    // Iterate over the lines of tokens and print them
    // to standard error

    // map of labels to line number

    map<string, int> labelMap;
    populateLabelMap(labelMap, tokLines);

    int lineNum = 0;
    
    vector<vector<Token*> >::iterator it;
    for(it = tokLines.begin(); it != tokLines.end(); ++it)
    {
      vector<Token*>::iterator it2;

      for(it2 = it->begin(); it2 != it->end(); ++it2)
      {
          // cout << (*it2)->getLexeme();
          ASM::Kind firstKind = (*it2)->getKind();
          string firstLexeme = (*it2)->getLexeme();
          
          if (firstKind == ASM::DOTWORD)
          {
              // .word instruction has 2 things
              if ( it->end() - it2 != 2) {
                  // throw ASMException(".word must have 2 arguments");
                  throw string("ERROR: .word must have 2 arguments");
              }
              ++ it2;

              // TODO: Do these checks in another function
              // get value of word: either decimal or hex
              if ( (*it2)->getKind() == ASM::INT || (*it2)->getKind() == ASM::HEXINT ) {
                  int wordValue = (*it2)->toInt();
                  printMipsInstruction(wordValue);
              }
              else if ( (*it2)->getKind()==ASM::ID ) {
                  std::map<string,int>::iterator mapit;
                  mapit = labelMap.find( (*it2)->getLexeme() );

                  if (mapit != labelMap.end()) {
                      int wordValue = mapit->second;

                      printMipsInstruction(wordValue);
                  } else {
                      throw string("ERROR: .word has invalid parameter");
                  }
              }
              else {
                  throw string("ERROR: .word has incorrect value");
              }

          }
          // OPCODE
          else if (firstKind == ASM::ID)
          {
              // jump register
              if ( (*it2)->getLexeme() == "jr")
              {
                  verifyJTypeInstruction(it, it2);
                  ++ it2;

                  // 1000 = 8
                  int instr = 8;
                  int jumpReg = (*it2)->toInt();
                  
                  printMipsInstruction( (jumpReg << 21) | instr);
              }
              // jump and load register
              else if ( firstLexeme == "jalr")
              {
                  verifyJTypeInstruction(it, it2);
                  ++ it2;
                  
                  // 1001 = 9
                  int instr = 9;
                  int jumpReg = (*it2)->toInt();

                  printMipsInstruction( (jumpReg << 21) | instr);
              }
              // add
              else if ( firstLexeme == "add" )
              {
                  verifyRTypeInstruction(it, it2);

                  int instr = 32;
                  int dREG = (*(it2+1))->toInt();
                  int sREG = (*(it2+3))->toInt();
                  int tREG = (*(it2+5))->toInt();
                  it2 += 5;
                  
                  printMipsInstruction( (sREG << 21) | (tREG << 16) | (dREG << 11) | instr );
                  
              }
              else if ( firstLexeme == "sub" )
              {
                  verifyRTypeInstruction(it, it2);

                  int instr = 34;
                  int dREG = (*(it2+1))->toInt();
                  int sREG = (*(it2+3))->toInt();
                  int tREG = (*(it2+5))->toInt();
                  it2 += 5;

                  printMipsInstruction( (sREG << 21) | (tREG << 16) | (dREG << 11) | instr );
                  
              }
              else if ( firstLexeme == "slt" )
              {
                  verifyRTypeInstruction(it, it2);

                  int instr = 42;
                  int dREG = (*(it2+1))->toInt();
                  int sREG = (*(it2+3))->toInt();
                  int tREG = (*(it2+5))->toInt();
                  it2 += 5;

                  printMipsInstruction( (sREG << 21) | (tREG << 16) | (dREG << 11) | instr );
                  
              }
              else if ( firstLexeme == "sltu" )
              {
                  verifyRTypeInstruction(it, it2);

                  int instr = 43;
                  int dREG = (*(it2+1))->toInt();
                  int sREG = (*(it2+3))->toInt();
                  int tREG = (*(it2+5))->toInt();
                  it2 += 5;

                  printMipsInstruction( (sREG << 21) | (tREG << 16) | (dREG << 11) | instr );
                  
              }
              // beq $s, $t, i
              else if ( firstLexeme == "beq")
              {
                  verifyITypeInstruction(it, it2);                  
                  int bValue = getBranchOffset(it2, labelMap, lineNum);

                  int instr = 4;
                  int sREG = (*(it2+1))->toInt();
                  int tREG = (*(it2+3))->toInt();
                  it2 += 5;

                  printMipsInstruction( (instr << 26) | (sREG << 21) | (tREG << 16) | (bValue & 0xffff));
              }
              // bne $s, $t, i
              else if ( firstLexeme == "bne") {
                  verifyITypeInstruction(it, it2);
                  int bValue = getBranchOffset(it2, labelMap, lineNum);

                  int instr = 5;
                  int sREG = (*(it2+1))->toInt();
                  int tREG = (*(it2+3))->toInt();
                  it2 += 5;

                  printMipsInstruction( (instr << 26) | (sREG << 21) | (tREG << 16) | (bValue & 0xffff));
              }
              else if ( firstLexeme == "lis")
              {
                  verifyJTypeInstruction(it, it2);
                  ++ it2;
                  
                  int instr = 20;
                  int dREG = (*it2)->toInt();
                  printMipsInstruction ( instr | (dREG << 11));
              }
              else if ( firstLexeme == "mflo")
              {
                  verifyJTypeInstruction(it, it2);
                  ++ it2;
                  
                  int instr = 18;
                  int dREG = (*it2)->toInt();

                  printMipsInstruction ( instr | (dREG << 11));
              }
              else if ( firstLexeme == "mfhi")
              {
                  verifyJTypeInstruction(it, it2);
                  ++ it2;
                  
                  int instr = 16;
                  int dREG = (*it2)->toInt();

                  printMipsInstruction ( instr | (dREG << 11));
              }
              else if ( firstLexeme == "mult" )
              {
                  verifyRTypeInstructionTwoParams(it, it2);
                  
                  int instr = 24;
                  int sREG = (*(it2+1))->toInt();
                  int tREG = (*(it2+3))->toInt();
                  it2 += 3;
                  printMipsInstruction( (sREG << 21) | (tREG << 16) | instr );
              }
              else if ( firstLexeme == "multu" )
              {
                  verifyRTypeInstructionTwoParams(it, it2);
                  int instr = 25;
                  int sREG = (*(it2+1))->toInt();
                  int tREG = (*(it2+3))->toInt();
                  it2 += 3;
                  printMipsInstruction( (sREG << 21) | (tREG << 16) | instr );
              }
              else if ( firstLexeme == "div" )
              {
                  verifyRTypeInstructionTwoParams(it, it2);
                  int instr = 26;
                  int sREG = (*(it2+1))->toInt();
                  int tREG = (*(it2+3))->toInt();
                  it2 += 3;
                  printMipsInstruction( (sREG << 21) | (tREG << 16) | instr );
              }
              else if ( firstLexeme == "divu" )
              {
                  verifyRTypeInstructionTwoParams(it, it2);
                  int instr = 27;
                  int sREG = (*(it2+1))->toInt();
                  int tREG = (*(it2+3))->toInt();
                  it2 += 3;
                  printMipsInstruction( (sREG << 21) | (tREG << 16) | instr );
              }
              // not a valid opcode
              else
              {
                  throw string("ERROR! Invalid Opcode");
                  
              }
          }
          else
          {
              std::ostringstream ss;
              ss << "ERROR! Not a valid MIPS Instruction:  " << (*it2)->getLexeme();
              throw ss.str();
          }
      }
      
      lineNum += 4;
     }

  } catch(const string& msg){
      // If an exception occurs print the message and end the program
      cerr << msg << endl;
  }
  // Delete the Tokens that have been made
  vector<vector<Token*> >::iterator it;
  for(it = tokLines.begin(); it != tokLines.end(); ++it){
      vector<Token*>::iterator it2;
      for(it2 = it->begin(); it2 != it->end(); ++it2){
          delete *it2;
      }
  }
}
