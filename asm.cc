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
    for(it = tokLines.begin(); it != tokLines.end(); ++it)
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
      
      if (!nullLine)
          lineNum += 4;
    }
}

// add, sub, slt, sltu
void verifyRTypeInstruction(vector<vector<Token*> >::iterator &it,
  vector<Token*>::iterator &it2) {

  if (it->end() - it2 != 6)
      throw string("ERROR! add has 3 arguments");

  // verify the next 3 params are registers
  if (!( (*(it2+1))->getKind() == ASM::REGISTER
         && (*(it2+2))->getKind() == ASM::COMMA
         && (*(it2+3))->getKind() == ASM::REGISTER
         && (*(it2+4))->getKind() == ASM::COMMA
         && (*(it2+5))->getKind() == ASM::REGISTER
          ) )
      throw string("ERROR! parameters must be registers");
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

              // get value of word: either decimal or hex
              if ( (*it2)->getKind() == ASM::INT || (*it2)->getKind() == ASM::HEXINT ) {
                  int wordValue = (*it2)->toInt();
                  printMipsInstruction(wordValue);
              }
              else if ( (*it2)->getKind()==ASM::ID ) {
                  std::map<string,int>::iterator mapit;
                  mapit = labelMap.find( (*it2)->getLexeme() );

                  cerr << (*it2)->getLexeme() << endl;
                  
                  if (mapit != labelMap.end()) {
                      int wordValue = mapit->second;
                      cerr << wordValue << endl;
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
                  if (it->end() - it2 != 2)
                      throw string("ERROR! Invalid number of arguments for jr");
                  ++ it2;
                  if ((*it2)->getKind() != ASM::REGISTER)
                      throw string("ERROR! Must give register for jr param");

                  int instr = 8;
                  int jumpReg = (*it2)->toInt();
                  
                  // 1000 = 8
                  printMipsInstruction( (jumpReg << 21) | instr);
              }
              // jump and load register
              else if ( firstLexeme == "jalr")
              {
                  if (it->end() - it2 != 2)
                      throw string("ERROR! Invalid number of arguments for jalr");
                  ++ it2;
                  if ((*it2)->getKind() != ASM::REGISTER)
                      throw string("ERROR! Must give register for jalr");

                  int instr = 9;
                  int jumpReg = (*it2)->toInt();

                  // 1001 = 9
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
