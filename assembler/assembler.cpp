#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <sstream>
#include <bitset>
#include <unordered_map>
#include <cctype>
#include <unordered_set>

using namespace std;

unordered_map<string, int> labels;
unordered_map<int, int> relLengths;

string fileName;
string inFileName;
string intFileName;
string outFileName;
ifstream inFile;
fstream intFile;
ofstream outFile;

int lineNumber = 0;

int calcLinesForInstruction(vector<string> words);
int calculateImmediate(string word);
int calculateImmediateLength(string word);
string toBinary(int num, int bits);
string toSignedImmediate(int num, int bits);
void shiftLabels(int startLine, int amount);
void labelPass();
void optimizationPass();
void instructionPass();

int main() {
    cin >> fileName;
    //filename = "test";
    inFileName = fileName + ".asm";
    intFileName = fileName + ".int";
    outFileName = fileName + ".bin";
    inFile.open(inFileName);
    intFile.open(intFileName);
    outFile.open(outFileName);

    labelPass();
    optimizationPass();
    instructionPass();

    return 0;
}

string toBinary(int num, int bits) {
    bitset<16> bin(num);
    return bin.to_string().substr(16 - bits);
}

string toSignedImmediate(int num, int bits) {
    bitset<16> bin(num);
    return bin.to_string().substr(16 - bits);
}

int calculateImmediateLength(int IMM) {
    int length = 1;
    if (abs(IMM) > 7)
        length++;
    if (abs(IMM) > 127)
        length++;
    if (abs(IMM) > 2047)
        length++;
    return length;
}

int calculateImmediate(string word) {
    int IMM;
    if (isdigit(word.at(0)) || word.at(0) == '-')
        IMM = stoi(word);
    else if (isalpha(word.at(0))) {
        IMM = labels.at(word) - lineNumber;
    }
    else if (word.at(0) == '$') {
        IMM = stoi(word.substr(1), nullptr, 16);
    }
    else if (word.at(0) == '%') {
        IMM = stoi(word.substr(1), nullptr, 2);
    }
    return IMM;
}

int calcLinesForInstruction(vector<string> words) {
    string op = words[0];
    int lines = 1;
    if (op == "IMM") {
        if (words.at(1) == "AB") {
            lines = calculateImmediateLength(calculateImmediate(words.at(2)));
        }
        else if (words.at(1) == "A") {
            lines = calculateImmediateLength(calculateImmediate(words.at(2)));
        }
        else if (words.at(1).at(0) == 'R') {
            lines = 1 + calculateImmediateLength(calculateImmediate(words.at(2)));
        }
    }
    else if (op.at(0) == 'B' || op.at(0) == 'J' || op == "CALL") {
        if (words.at(1) == "REL" || words.at(1) == "ABS") {
            //do nothing, lines = 1
        }
        else {
            lines = 1 + calculateImmediateLength(calculateImmediate(words.at(1)));
        }
    }
    else if (op == "STO" || op == "LD") {
        if (words.size() == 3) {
            lines = 1 + calculateImmediateLength(calculateImmediate(words.at(2)));
        }
    }
    else if (op == "MV" && words.at(1) == "AB" && words.size() == 4) {
        lines = 2;
    }
    else if ((op == "PUSH" || op == "POP") && (words.at(1) == "AB" || words.at(1) == "RA")) {
        lines = 2;
    }
    return lines;
}

void shiftLabels(int startLine, int amount) {
    for (auto& [key, value] : labels) {
        if (value >= startLine) 
            value += amount;
    }
    for (auto& [key, value] : relLengths) {
        if (key >= startLine) {
            int keyVal = key;
            relLengths.erase(key);
            relLengths.insert_or_assign(keyVal + amount, value);
        }
    }
}

void labelPass() {
    string line;
    lineNumber = 0;
    while (getline(inFile, line)) {  
        if (line[0] == ':') {
            labels.insert_or_assign(line.substr(1), lineNumber);
        }
        else if (line.empty() || line.substr(0, 2) == "//") {
            continue; // Skip empty lines and comments
        }
        else {
            istringstream iss(line);
            vector<string> words;
            string word;
            while (iss >> word) { //delete inline comments
                if (word.substr(0,2) != "//") {
                    words.push_back(word);
                    intFile << word << " ";
                }
            }
            lineNumber += calcLinesForInstruction(words);
            intFile << "\n";
        }
    }
    inFile.close();
}

void optimizationPass() {
    string line;
    lineNumber = 0;
    istringstream iss(line);
    vector<string> words;
    string word;
    int prevLength = -1;
    int length = 0;
    while (prevLength != length) {
        lineNumber = 0;
        while (getline(intFile, line)) {
            while (iss >> word) {
                words.push_back(word);
            }
            for (auto& [key, value] : relLengths) {
                int shiftAmount = value - calcLinesForInstruction(words);
                if (shiftAmount > 0) {
                    shiftLabels(key, shiftAmount);
                }
            }
        }
        prevLength = length;
        length = lineNumber;
    }
}

void instructionPass() {
    string line;
    lineNumber = 0;
    istringstream iss(line);
    vector<string> words;
    string word;
    while (getline(intFile, line)) {
        while (iss >> word) {
            words.push_back(word);
        }
        string op = words[0];
        if (op == "IMM") {
            if (words.at(1) == "AB") {
                for (int i = 0; i < calcLinesForInstruction(words) - 1; i += 1)
                    outFile << "00" << toBinary(i, 2) << toSignedImmediate(calculateImmediate(words.at(2)), 16).substr(i * 4, 4) << "\n";
            }
            else if (words.at(1) == "A") {
                for (int i = 0; i < calcLinesForInstruction(words) - 1; i += 1)
                    outFile << "00" << toBinary(i, 2) << toSignedImmediate(calculateImmediate(words.at(2)), 8).substr(i * 4, 4) << "\n";
            }    
            else if (words.at(1).at(0) == 'R') {
                for (int i = 0; i < calcLinesForInstruction(words) - 1; i += 1)
                    outFile << "00" << toBinary(i, 2) << toSignedImmediate(calculateImmediate(words.at(2)), 8).substr(i * 4, 4) << "\n";
                outFile << "110001" << toBinary(stoi(words.at(1).substr(1)), 2) << "\n";
            }
        }
        else if (op == "MV") {
            if (words.at(1) == "A") {
                if (words.at(2).at(0) == 'R') {
                    outFile << "110000" << toBinary(stoi(words.at(2).substr(1)), 2) << "\n";
                }
                else if (words.at(2) == "IR")
                    outFile << "11101110" << "\n";
            }
            else if (words.at(1).at(0) == 'R') {
                if (words.at(2) == "A") {
                    outFile << "110001" << toBinary(stoi(words.at(1).substr(1)), 2) << "\n";
                }
            }
            else if (words.at(1) == "AB" || words.at(1) == "AB_bot") {
                if (words.size() == 4) {
                    outFile << "110110" << toBinary(stoi(words.at(2).substr(1)), 2) << "\n";
                    outFile << "110111" << toBinary(stoi(words.at(3).substr(1)), 2) << "\n";
                }
                else {
                    if (words.at(2).at(0) == 'R') {
                        outFile << "110110" << toBinary(stoi(words.at(2).substr(1)), 2) << "\n";
                    }
                    else if (words.at(2) == "SP") {
                        outFile << "11111001" << "\n";
                    }
                }
            }
            else if (words.at(1) == "AB_top") {
                outFile << "110111" << toBinary(stoi(words.at(2).substr(1)), 2) << "\n";
            }
            else if (words.at(1) == "IR") {
                outFile << "11101111" << "\n";
            }
            else if (words.at(1) == "SP") {
                outFile << "11111000" << "\n";
            }
            else if (words.at(1) == "IJA") {
                outFile << "11111111" << "\n";
            }
            else if (words.at(1) == "PB") {
                outFile << "11101100" << "\n";
            }
            else if (words.at(1) == "PL") {
                outFile << "11111011" << "\n";
            }
             else if (words.at(1) == "IRA") {
                outFile << "11111101" << "\n";
            }
        }
        else if (op == "STO") {
            if (words.size() == 3) {
                for (int i = 0; i < calcLinesForInstruction(words) - 1; i += 1)
                    outFile << "00" << toBinary(i, 2) << toSignedImmediate(calculateImmediate(words.at(2)), 16).substr(i * 4, 4) << "\n";
                if (words.at(1).at(0) == 'R') {
                    outFile << "110011" << toBinary(stoi(words.at(1).substr(1)), 2) << "\n";
                }
                else if (words.at(1) == "A") {
                    outFile << "11110001" << "\n";
                }
            }
            else if (words.at(1).at(0) == 'R') {
                outFile << "110011" << toBinary(stoi(words.at(1).substr(1)), 2) << "\n";
            }
            else if (words.at(1) == "A") {
                outFile << "11110001" << "\n";
            }
        }
        else if (op == "LD") {
            if (words.size() == 3) {
                for (int i = 0; i < calcLinesForInstruction(words) - 1; i += 1)
                    outFile << "00" << toBinary(i, 2) << toSignedImmediate(calculateImmediate(words.at(2)), 16).substr(i * 4, 4) << "\n";
                if (words.at(1).at(0) == 'R') {
                    outFile << "110010" << toBinary(stoi(words.at(1).substr(1)), 2) << "\n";
                }
                else if (words.at(1) == "A") {
                    outFile << "11110000" << "\n";
                }
            }
            else if (words.at(1).at(0) == 'R') {
                outFile << "110010" << toBinary(stoi(words.at(1).substr(1)), 2) << "\n";
            }
            else if (words.at(1) == "A") {
                outFile << "11110000" << "\n";
            }
        }
        else if (op == "ADD") {
            if (words.size() == 2) {
                outFile << "100000" << toBinary(stoi(words.at(1).substr(1)), 2) << "\n";
            }
            else {
                if (words.at(1).at(0) == 'R') {
                    outFile << "11010" << (stoi(words.at(2)) + 1) / 2 << toBinary(stoi(words.at(1).substr(1)), 2) <<"\n";
                }
                else if (words.at(1) == "AB") {
                    if (words.at(2) == "A") {
                        outFile << "11111110" << "\n";
                    }
                    else {
                        outFile << "" << (stoi(words.at(2)) + 1) / 2 << "\n"; 
                    }                    
                }
            }
        }
        else if (op == "PUSH") {
            if (words.at(1) == "A") {
                outFile << "11110011" << "\n";
            }
            else if (words.at(1) == "AB") {
                outFile << "10111110" << "\n";
                outFile << "10111111" << "\n";
            }
            else if (words.at(1) == "AB_bot") {
                outFile << "10111110" << "\n";
            }
            else if (words.at(1) == "FR") {
                outFile << "11110101" << "\n";
            }
            else if (words.at(1).at(0) == 'R') {
                outFile << "101101" << toBinary(stoi(words.at(1).substr(1)), 2) << "\n";
            }
            else if (words.at(1) == "RA") {
                outFile << "10111100" << "\n";
                outFile << "10111101" << "\n";
            }
            else if (words.at(1) == "RA_bot") {
                outFile << "10111100" << "\n";
            }
        }
        else if (op == "PUSH") {
            if (words.at(1) == "A") {
                outFile << "11110010" << "\n";
            }
            else if (words.at(1) == "AB") {
                outFile << "10111010" << "\n";
                outFile << "10111011" << "\n";
            }
            else if (words.at(1) == "AB_bot") {
                outFile << "10111010" << "\n";
            }
            else if (words.at(1) == "FR") {
                outFile << "11110100" << "\n";
            }
            else if (words.at(1).at(0) == 'R') {
                outFile << "101100" << toBinary(stoi(words.at(1).substr(1)), 2) << "\n";
            }
            else if (words.at(1) == "RA") {
                outFile << "10111000" << "\n";
                outFile << "10111001" << "\n";
            }
            else if (words.at(1) == "RA_bot") {
                outFile << "10111000" << "\n";
            }
        }
        else if (op.at(0) == 'B') {
            bool isRel = false;
            if (words.at(1) == "REL")
                isRel = true;
            else if (words.at(1) == "ABS")
                isRel = false;
            else {
                if (words.at(1).at(0) == ':')
                    isRel = true;
                else
                    isRel = false;
                for (int i = 0; i < calcLinesForInstruction(words) - 1; i += 1)
                    outFile << "00" << toBinary(i, 2) << toSignedImmediate(calculateImmediate(words.at(1)), 16).substr(i * 4, 4) << "\n";
            }
            if (op == "BEQ") {
                outFile << "011000" << (isRel ? '0' : '1') << "0" << "\n";
            }
            else if (op == "BNE") {
                outFile << "011001" << (isRel ? '0' : '1') << "0" << "\n";
            }
            else if (op == "BLTU" || op == "BORROW") {
                outFile << "011010" << (isRel ? '0' : '1') << "0" << "\n";
            }
            else if (op == "BLTS") {
                outFile << "011011" << (isRel ? '0' : '1') << "0" << "\n";
            }
            else if (op == "BGTEU" || op == "CARRY") {
                outFile << "011100" << (isRel ? '0' : '1') << "0" << "\n";
            }
            else if (op == "BGTES") {
                outFile << "011101" << (isRel ? '0' : '1') << "0" << "\n";
            }
        }
        else if (op.at(0) == 'J') {
            bool isRel = false;
            if (words.at(1) == "REL")
                isRel = true;
            else if (words.at(1) == "ABS")
                isRel = false;
            else {
                if (words.at(1).at(0) == ':')
                    isRel = true;
                else
                    isRel = false;
                for (int i = 0; i < calcLinesForInstruction(words) - 1; i += 1)
                    outFile << "00" << toBinary(i, 2) << toSignedImmediate(calculateImmediate(words.at(1)), 16).substr(i * 4, 4) << "\n";
            }
            outFile << "011111" << (isRel ? '0' : '1') << "0" << "\n";
        }
        else if (op == "CAll") {
            bool isRel = false;
            if (words.at(1) == "REL")
                isRel = true;
            else if (words.at(1) == "ABS")
                isRel = false;
            else {
                if (words.at(1).at(0) == ':')
                    isRel = true;
                else
                    isRel = false;
                for (int i = 0; i < calcLinesForInstruction(words) - 1; i += 1)
                    outFile << "00" << toBinary(i, 2) << toSignedImmediate(calculateImmediate(words.at(1)), 16).substr(i * 4, 4) << "\n";
            }
            outFile << "011111" << (isRel ? '0' : '1') << "1" << "\n";
        }
        else if (op == "SUB") {
            outFile << "100001" << toBinary(stoi(words.at(1).substr(1)), 2) << "\n";
        }
        else if (op == "SLA") {
            if (words.at(1).at(0) == 'R') {
                outFile << "100010" << toBinary(stoi(words.at(1).substr(1)), 2) << "\n";
            }
            else if (isdigit(words.at(1).at(0))) {
                outFile << "10100" << toBinary(stoi(words.at(1)), 3) << "\n";
            }        
        }
        else if (op == "SRA") {
            if (words.at(1).at(0) == 'R') {
                outFile << "100011" << toBinary(stoi(words.at(1).substr(1)), 2) << "\n";
            }
            else if (isdigit(words.at(1).at(0))) {
                outFile << "10101" << toBinary(stoi(words.at(1)), 3) << "\n";
            }        
        }
        else if (op == "SRL") {
            outFile << "100100" << toBinary(stoi(words.at(1).substr(1)), 2) << "\n";
        }
        else if (op == "AND") {
            outFile << "100101" << toBinary(stoi(words.at(1).substr(1)), 2) << "\n";
        }
        else if (op == "OR") {
            outFile << "100110" << toBinary(stoi(words.at(1).substr(1)), 2) << "\n";
        }
        else if (op == "XOR") {
            outFile << "100111" << toBinary(stoi(words.at(1).substr(1)), 2) << "\n";
        }
        else if (op == "NOP") {
            outFile << "11111100" << "\n";
        }
        else if (op == "RET") {
            outFile << "11111010" << "\n";
        }
        else if (op == "IRET") {
            outFile << "11101101" << "\n";
        }
        else if (op == "USER") {
            outFile << "11101010" << "\n";
        }
        else if (op == "KERNEL") {
            outFile << "11101011" << "\n";
        }
        else if (op == "SYSCALL") {
            outFile << "11101111" << "\n";
        }
        lineNumber += calcLinesForInstruction(words);
    }
    outFile.close();
    intFile.close();
}   