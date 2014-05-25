#include "kind.h"
#include "lexer.h"
#include <vector>
#include <string>
#include <iostream>
// Use only the neeeded aspects of each namespace
using std::string;
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

    vector<vector<Token*> >::iterator it;
    for(it = tokLines.begin(); it != tokLines.end(); ++it){
      vector<Token*>::iterator it2;
      
      for(it2 = it->begin(); it2 != it->end(); ++it2){
          // cout << (*it2)->getLexeme() << endl;
          
          if ((*it2)->getKind() == ASM::DOTWORD) {
              try {
                  // .word instruction has 2 things
                  if ( it->end() - it2 != 2) {
                      throw ASMException(".word must have 2 arguments");
                  }
                  ++ it2;
                  // get value of word: either decimal or hex
                  if (! ((*it2)->getKind() == ASM::INT || (*it2)->getKind() == ASM::HEXINT)) {
                      throw ASMException("Word must be int or hex");
                  }

                  int wordValue = (*it2)->toInt();
                  printMipsInstruction(wordValue);
                  
              } catch (std::exception &e) {
                  cerr << "ERROR: " << e.what() << endl;
              }
          }
      }
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
