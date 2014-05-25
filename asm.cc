#include "kind.h"
#include "lexer.h"
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <map>
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

void populateLabelMap(std::map<string, int> &labelMap, vector< vector<Token*> > &tokLines) {
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

              //Labels can only be defined once
              if (labelMap.find(label) == labelMap.end()) {
                  labelMap[label] = lineNum;
              } else {
                  throw string("ERROR: Duplicate label definition");
              }

              if (it2 > it->begin()) {
                  //throw exception if the thing before it is an integer
                  if ( (*(it2-1))->getKind() == ASM::INT ) {
                      throw string("ERROR: Not a proper label, must not have leading numbers");
                  }
              }

              //erase the label, and get next element
              it2 = (*it).erase(it2);
          }
      }
      
      if (!nullLine)
          lineNum += 4;
    }
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

    std::map<string, int> labelMap;
    populateLabelMap(labelMap, tokLines);

    int lineNum = 0;
    
    vector<vector<Token*> >::iterator it;
    for(it = tokLines.begin(); it != tokLines.end(); ++it)
    {
      vector<Token*>::iterator it2;

      for(it2 = it->begin(); it2 != it->end(); ++it2)
      {
          // cout << (*it2)->getLexeme();
          
          if ((*it2)->getKind() == ASM::DOTWORD) {
              // .word instruction has 2 things
              if ( it->end() - it2 != 2) {
                  // throw ASMException(".word must have 2 arguments");
                  throw string("ERROR: .word must have 2 arguments");
              }
              ++ it2;
              // get value of word: either decimal or hex
              if (!((*it2)->getKind() == ASM::INT || (*it2)->getKind() == ASM::HEXINT)) {
                  throw string("ERROR: .word must be decimal or hex");
              }

              int wordValue = (*it2)->toInt();
              printMipsInstruction(wordValue);
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

    //print out label map
    for (std::map<string, int>::iterator it = labelMap.begin(); it != labelMap.end(); ++it) {
        cerr << it->first << " " << it->second << endl;
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
