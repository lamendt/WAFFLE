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

struct symbolCall {
    int lineNumber;
    int size;
};

unordered_map<string, int> labels;
vector<symbolCall> relLengths;

string fileName;
string inFileName;
string intFileName;
string outFileName;
ifstream inFile;
fstream intFile;
ofstream outFile;

int lineNumber = 0;

int calcLinesForInstruction(vector<string> words, int ln);
int calculateImmediate(string word);
int calculateImmediate(string word, int ln);
int calculateImmediateLength(string word);
int calculateImmediateLength(string word, int ln);
void writeImmediate(string word, int ln, ofstream& binFile);
void writeImmediate(string word, ofstream& binFile);
void writeImmediate(string word, ofstream& binFile, string r1, string r2);
string toBinary(int num, int bits);
void shiftLabels(int startLine, int amount);
void labelPass();
void optimizationPass();
void instructionPass();

int main() {
    cin >> fileName;
    //fileName = "test";
    inFileName = "tests/" + fileName + ".asm";
    intFileName = "tests/" + fileName + ".int";
    outFileName = "tests/" + fileName + ".bin";
    inFile.open(inFileName);
    intFile.open(intFileName, ios::in | ios::out | ios::trunc);
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

int calculateImmediateLength(string word) {
    int IMM = calculateImmediate(word);
    int length = 1;
    if (IMM > 7 || IMM < -8)
        length++;
    if (IMM > 127 || IMM < -128)
        length++;
    if (IMM > 2047 || IMM < -2048)
        length++;
    return length;
}

int calculateImmediateLength(string word, int ln) {
    int IMM = calculateImmediate(word, ln);
    int length = 1;
    if (IMM > 7 || IMM < -8)
        length++;
    if (IMM > 127 || IMM < -128)
        length++;
    if (IMM > 2047 || IMM < -2048)
        length++;
    return length;
}

int calculateImmediate(string word) {
    int IMM;
    if (isdigit(word.at(0)) || word.at(0) == '-')
        IMM = stoi(word);
    else if (isalpha(word.at(0))) {
        if (labels.count(word)) {
            IMM = labels.at(word);
        }
        else {
            IMM = 3000;
        }
    }
    else if (word.at(0) == '$') {
        IMM = stoi(word.substr(1), nullptr, 16);
    }
    else if (word.at(0) == '%') {
        IMM = stoi(word.substr(1), nullptr, 2);
    }
    return IMM;
}

int calculateImmediate(string word, int ln) {
    int IMM;
    if (labels.count(word)) {
        IMM = labels.at(word) - ln;
        for (auto& i : relLengths) {
            if (i.lineNumber == ln) {
                IMM -= (i.size - 1);
            }
        }
    }
    else {
        IMM = 3000;
    }
    return IMM;
}

void writeImmediate(string word, int ln, ofstream& binFile) {
    for (int i = 0; i < calculateImmediateLength(word, ln); i += 1)
        binFile << "00" << toBinary(i, 2) << toBinary(calculateImmediate(word, ln), 16).substr(12-i * 4, 4) << "\n";
}

void writeImmediate(string word, ofstream& binFile) {
    for (int i = 0; i < calculateImmediateLength(word); i += 1)
        binFile << "00" << toBinary(i, 2) << toBinary(calculateImmediate(word), 16).substr(12-i * 4, 4) << "\n";
}

void writeImmediate(string word, ofstream& binFile, string r1, string r2) {
    binFile << "010" << 0 << toBinary(calculateImmediate(word), 16).substr(0, 4) << "\n";
    binFile << "010" << 1 << toBinary(calculateImmediate(word), 16).substr(4, 4) << "\n";
    binFile << "110001" << toBinary(r1.at(1), 2) << "\n";
    binFile << "010" << 0 << toBinary(calculateImmediate(word), 16).substr(8, 4) << "\n";
    binFile << "010" << 1 << toBinary(calculateImmediate(word), 16).substr(12, 4) << "\n";
    binFile << "110001" << toBinary(r2.at(1), 2) << "\n";
}

int calcLinesForInstruction(vector<string> words, int ln) {
    string op = words.at(0);
    int lines = 1;
    if (isupper(op.at(0))) {
        if (op == "IMM") {
            if (words.at(1) == "AB") {
                lines = calculateImmediateLength(words.at(2));
            }
            else if (words.at(1) == "A") {
                lines = calculateImmediateLength(words.at(2));
            }
            else if (words.at(1).at(0) == 'R') {
                lines = 1 + calculateImmediateLength(words.at(2));
            }
        }
        else if (op.at(0) == 'B' || op.at(0) == 'J' || op == "CALL") {
            if (words.at(1) == "REL" || words.at(1) == "ABS") {
                //do nothing, lines = 1
            }
            else {
                lines = 1 + calculateImmediateLength(words.at(1), ln); 
            }
        }
        else if (op == "STO" || op == "LD") {
            if (words.size() == 3) {
                lines = 1 + calculateImmediateLength(words.at(2));
            }
        }
        else if (op == "MV" && words.at(1) == "AB" && words.size() == 4) {
            lines = 2;
        }
        else if ((op == "PUSH" || op == "POP") && (words.at(1) == "AB" || words.at(1) == "RA")) {
            lines = 2;
        }
        else if (op == "ADR") {
            if (words.size() == 4) {
                lines = 6;
            }
            else {
                lines = calculateImmediateLength(words.at(1));
            }
        }
    }
    else if (op.at(0) != '\'') {
        int len = calculateImmediateLength(op.substr(1));
        if (len < 3)
            lines = 1;
        else
            lines = 2;
    }
    else {
        lines = 0;
        string str = "";
        for (int i = 0; i < words.size(); i++) {
            str += words.at(i) + " ";
        }
        str = str.substr(1, str.size() - 2);
        for (int i = 0; i < str.size(); i++) {
            if (str.at(i) != '\\' || (str.at(i) == '\\' && str.at(i+1) == '\\'))
                lines++;    
        }
    }
    return lines;
}

void shiftLabels(int startLine, int amount) {
    for (auto& [key, value] : labels) {
        if (value > startLine) 
            value -= amount;
    }
    for (auto& value : relLengths) {
        if (value.lineNumber > startLine) {
            value.lineNumber -= amount;
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
            bool inlineComment = false;
            bool relLength = false;
            while (iss >> word) {
                if (word.substr(0,2) == "//") {
                    inlineComment = true;
                }
                if (!inlineComment) {
                    words.push_back(word);
                    intFile << word << " ";
                    if (islower(word.at(0))) {
                        relLength = true;
                    }
                }
            }
            int lines = calcLinesForInstruction(words, lineNumber);
            if (relLength && isupper(words.at(0).at(0)))
                relLengths.push_back({lineNumber, lines});
            lineNumber += lines;
            intFile << "\n";
        }
    }
    inFile.close();
}

void optimizationPass() {
    string line;
    lineNumber = 0;
    string word;
    int prevLength = -1;
    int length = 0;
    while (prevLength != length) {
        lineNumber = 0;
        intFile.clear();            // clear EOF flag
        intFile.seekg(0);           // rewind to beginning
        while (getline(intFile, line)) {
            istringstream iss(line);
            vector<string> words;
            while (iss >> word) {
                words.push_back(word);
            }
            for (auto& i : relLengths) {
                if (i.lineNumber == lineNumber) {
                    int shiftAmount = i.size - calcLinesForInstruction(words, lineNumber);
                    i.size -= shiftAmount;
                    if (shiftAmount > 0) {
                        shiftLabels(lineNumber, shiftAmount);
                    }
                }
            }
            lineNumber += calcLinesForInstruction(words, lineNumber);
        }
        prevLength = length;
        length = lineNumber;
    }
}

void instructionPass() {
    intFile.clear();            // clear EOF flag
    intFile.seekg(0);           // rewind to beginning
    string line;
    lineNumber = 0;
    string word;
    while (getline(intFile, line)) {
        istringstream iss(line);
        vector<string> words;
        while (iss >> word) {
            words.push_back(word);
        }
        string op = words.at(0);
        if (isupper(op.at(0))) {
            if (op == "IMM") {
                if (words.at(1) == "AB") {
                    writeImmediate(words.at(2), outFile);
                }
                else if (words.at(1) == "A") {
                    for (int i = 0; i < calculateImmediateLength(words.at(2)); i += 1)
                        outFile << "010" << toBinary(i, 1) << toBinary(calculateImmediate(words.at(2)), 8).substr(4-i * 4, 4) << "\n";
                }    
                else if (words.at(1).at(0) == 'R') {
                    for (int i = 0; i < calculateImmediateLength(words.at(2)); i += 1)
                        outFile << "010" << toBinary(i, 1) << toBinary(calculateImmediate(words.at(2)), 8).substr(4-i * 4, 4) << "\n";
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
                    writeImmediate(words.at(2), outFile);
                    if (words.at(2).at(0) == ':') {
                        if (words.at(1).at(0) == 'R') {
                            outFile << "111001" << toBinary(stoi(words.at(1).substr(1)), 2) << "\n";
                        }
                        else if (words.at(1) == "A") {
                            outFile << "11101001" << "\n";
                        }
                    }
                    else {
                        if (words.at(1).at(0) == 'R') {
                            outFile << "110011" << toBinary(stoi(words.at(1).substr(1)), 2) << "\n";
                        }
                        else if (words.at(1) == "A") {
                            outFile << "11110001" << "\n";
                        }
                    }

                }
                else if (words.at(1).at(0) == 'R') {
                    outFile << "111001" << toBinary(stoi(words.at(1).substr(1)), 2) << "\n";
                }
                else if (words.at(1) == "A") {
                    outFile << "11101001" << "\n";
                }
            }
            else if (op == "LD") {
                if (words.size() == 3) {
                    writeImmediate(words.at(2), outFile);
                    if (words.at(2).at(0) == ':') {
                        if (words.at(1).at(0) == 'R') {
                            outFile << "111000" << toBinary(stoi(words.at(1).substr(1)), 2) << "\n";
                        }
                        else if (words.at(1) == "A") {
                            outFile << "11101000" << "\n";
                        }
                    }
                    else {
                        if (words.at(1).at(0) == 'R') {
                            outFile << "110010" << toBinary(stoi(words.at(1).substr(1)), 2) << "\n";
                        }
                        else if (words.at(1) == "A") {
                            outFile << "11110000" << "\n";
                        }
                    }

                }
                else if (words.at(1).at(0) == 'R') {
                    outFile << "111000" << toBinary(stoi(words.at(1).substr(1)), 2) << "\n";
                }
                else if (words.at(1) == "A") {
                    outFile << "11101000" << "\n";
                }
            }
            else if (op == "ADR") {
                if (words.size() == 4) {
                    writeImmediate(words.at(3), outFile, words.at(1), words.at(2));
                }
                else {
                    writeImmediate(words.at(1), outFile);
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
                            outFile << "1111011" << (stoi(words.at(2)) + 1) / 2 << "\n"; 
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
            else if (op == "POP") {
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
                    writeImmediate(words.at(1), lineNumber, outFile);
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
                    writeImmediate(words.at(1), lineNumber, outFile);
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
                    writeImmediate(words.at(1), lineNumber, outFile);
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
            else if (op == "IJA") {
                outFile << "11111111" << "\n";
            }
            else if (op == "PB") {
                outFile << "11101100" << "\n";
            }
            else if (op == "PL") {
                outFile << "11111011" << "\n";
            }
            else if (op == "IRA") {
                outFile << "11111101" << "\n";
            }
        }
        else if (op.at(0) == '\'') {
            string str = "";
            for (int i = 0; i < words.size(); i++) {
                str += words.at(i) + " ";
            }
            str = str.substr(1, str.size() - 2);
            for (int i = 0; i < str.size(); i++) {
                if (str.at(i) != '\\')
                    outFile << toBinary(str.at(i), 8) << "\n";
                else if (str.substr(i,2) == "\\")
                    outFile << toBinary('\\', 8) << "\n";
            } 
        }
        else {
            if (calculateImmediateLength(op) < 3) {
                outFile << toBinary(calculateImmediate(op), 8) << "\n";
            }
            else {
                outFile << toBinary(calculateImmediate(op), 16).substr(0, 8) << "\n";
                outFile << toBinary(calculateImmediate(op), 16).substr(8, 8) << "\n";
            }
        }
        lineNumber += calcLinesForInstruction(words, lineNumber);
    }
    outFile.close();
    intFile.close();
}   