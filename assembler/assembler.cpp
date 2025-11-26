#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <sstream>
#include <bitset>
#include <unordered_map>
#include <cctype>

using namespace std;

int main() {
    stringstream workingFile;
    unordered_map<string, int> labels;
    
    string filename;
    cin >> filename;

    string infileName = filename + ".asm";
    string binfileName = filename + ".bin";
    ifstream infile(infileName);

    string line;
    int lineNumber = 0;

    while (getline(infile, line)) {
        
        if (line.empty() || line.substr(0, 2) == "//") {
            continue; // Skip empty lines and comments
        }
        else if (line[0] == ':') {
            // Handle label
            labels.insert_or_assign(line.substr(1), lineNumber);
            continue;
        }
        else {
            workingFile << line << "\n";
            istringstream iss(line);
            vector<string> words;
            string word;
            while (iss >> word)
                words.push_back(word);
            string op = words[0];

            if (op == "IMM") {
                if (words.at(1) == "AB") {
                    lineNumber += 3;
                }
                else if (words.at(1) == "A") {
                    lineNumber += 1;
                }    
            }
            else if (op == "J") {
                lineNumber += 4;
            }
            lineNumber++;
        }
    }

    infile.close();
    ofstream binfile(binfileName);
    lineNumber = 0;

    while (getline(workingFile, line)) {
        // Process instruction
        istringstream iss(line);
        vector<string> words;
        string word;
        while (iss >> word)
            words.push_back(word);
        string op = words[0];
        if (op == "IMM") {
            if (words.at(1) == "AB") {
                bitset<16> IMM = stoi(words.at(2));
                binfile << "0000" << IMM.to_string().substr(12) << "\n";
                binfile << "0001" << IMM.to_string().substr(8, 4) << "\n";
                binfile << "0010" << IMM.to_string().substr(4, 4) << "\n";
                binfile << "0011" << IMM.to_string().substr(0, 4) << "\n";
                lineNumber += 3;
            }
            else if (words.at(1) == "A") {
                bitset<8> IMM = stoi(words.at(2));
                binfile << "0100" << IMM.to_string().substr(4) << "\n";
                binfile << "0101" << IMM.to_string().substr(0,4) << "\n";
                lineNumber += 1;
            }    
        }
        else if (op == "MV") {
            if (words.at(1) == "A") {
                if (words.at(2).at(0) == 'R') {
                    bitset<2> reg = stoi(words.at(2).substr(1));
                    binfile << "110000" << reg.to_string() << "\n";
                }
                if (words.at(2) == "IR")
                    binfile << "11101110" << "\n";
            }
            else if (words.at(1).at(0) == 'R') {
                bitset<2> reg = stoi(words.at(1).substr(1));
                if (words.at(2) == "A")
                    binfile << "110001" << reg.to_string() << "\n";
            }
            else if (words.at(1) == "AB" || words.at(1) == "AB_bot") {
                if (words.at(2).at(0) == 'R') {
                    bitset<2> reg = stoi(words.at(2).substr(1));
                    binfile << "110110" << reg.to_string() << "\n";
                }
                if (words.at(2) == "A") {
                    binfile << "11101100" << "\n";
                }
                if (words.at(2) == "SP") {
                    binfile << "11111001" << "\n";
                }
            }
             else if (words.at(1) == "AB_top") {
                if (words.at(2).at(0) == 'R') {
                    bitset<2> reg = stoi(words.at(2).substr(1));
                    binfile << "110111" << reg.to_string() << "\n";
                }
                if (words.at(2) == "A") {
                    binfile << "11101101" << "\n";
                }
            }
            else if (words.at(1) == "IR") {
                if (words.at(2) == "A") {
                    binfile << "11101111" << "\n";
                }
            }
            else if (words.at(1) == "SP") {
                if (words.at(2) == "AB") {
                    binfile << "11111000" << "\n";
                }
            }
            else if (words.at(1) == "IJA") {
                if (words.at(2) == "AB") {
                    binfile << "11111111" << "\n";
                }
            }
        } 
        else if (op == "ADD") {
            if (words.size() == 2) {
                bitset<2> reg = stoi(words.at(1).substr(1));
                binfile << "100000" << reg.to_string() << "\n";

            }
            else {
                if (words.at(1) == "A") {
                        bitset<2> reg = stoi(words.at(2).substr(1));
                        binfile << "100000" << reg.to_string() << "\n";
                }
                else if (words.at(1).at(0) == 'R') {
                    if (words.at(2) == "-1" || words.at(2) == "1") {
                        bitset<2> reg = stoi(words.at(1).substr(1));
                        binfile << "11010" << (stoi(words.at(2)) + 1) / 2 << reg.to_string() << "\n";
                    }
                }
                else if (words.at(1) == "AB") {
                    if (words.at(2) == "-1" || words.at(2) == "1") {
                        binfile << "1111011" << (stoi(words.at(2)) + 1) / 2 << "00" << "\n";
                    }
                    else if (words.at(2).at(0) == 'R') {
                        bitset<2> reg = stoi(words.at(2).substr(1));
                        binfile << "111010" << "00" << reg.to_string() << "\n";
                    }
                    else if (words.at(2) == "A") {
                        binfile << "11111110" << "\n";
                    }
                }
            }
        }
        else if (op == "B") {
            lineNumber += 4;
            bitset<16> IMM;
            if (isalpha(words.at(3).at(0)))
                IMM = labels.at(words.at(3)) - lineNumber;
            else if (isdigit(words.at(3).at(0)))
                IMM = stoi(words.at(3));
            binfile << "0000" << IMM.to_string().substr(12) << "\n";
            binfile << "0001" << IMM.to_string().substr(8, 4) << "\n";
            binfile << "0010" << IMM.to_string().substr(4, 4) << "\n";
            binfile << "0011" << IMM.to_string().substr(0, 4) << "\n";
            if (words.at(1) == "EQ") {
                binfile << "011000";
            }
            else if (words.at(1) == "NEQ") {
                binfile << "011001";
            }
            else if (words.at(1) == "LTU" || words.at(1) == "BORROW" || words.at(1) == "NOCARRY") {
                binfile << "011010";
            }
            else if (words.at(1) == "LTS") {
                binfile << "011011";
            }
            else if (words.at(1) == "GTEU" || words.at(1) == "CARRY" || words.at(1) == "NOBORROW") {
                binfile << "011100";
            }
            else if (words.at(1) == "GTES") {
                binfile << "011101";
            }
            else if (words.at(1) == "J") {
                binfile << "011111";
            }
            if (isalpha(words.at(3).at(0)))
                binfile << "1";
            else if (isdigit(words.at(3).at(0)))
                binfile << "0";
            if (words.at(3) == "LINK") {
                binfile << "1\n";
            }
            else if (words.at(2) == "NOLINK") {
                binfile << "0\n";
            }
        }
        else if (op == "J" || op == "JL") {
            lineNumber += 4;
            bitset<16> IMM;
            if (isalpha(words.at(1).at(0)))
                IMM = labels.at(words.at(1)) - lineNumber;
            else if (isdigit(words.at(1).at(0)))
                IMM = stoi(words.at(1));
            binfile << "0000" << IMM.to_string().substr(12) << "\n";
            binfile << "0001" << IMM.to_string().substr(8, 4) << "\n";
            binfile << "0010" << IMM.to_string().substr(4, 4) << "\n";
            binfile << "0011" << IMM.to_string().substr(0, 4) << "\n";
            if (op == "J")
                binfile << "01111110";
            else if (op == "JL")
                binfile << "01111111";
        }
        else if (op == "STO") {
            if (words.at(1).at(0) == 'R') {
                bitset<2> reg = stoi(words.at(1).substr(1));
                binfile << "110011" << reg.to_string() << "\n";
            }
            else if (words.at(1) == "A") {
                binfile << "11110001";
            }
        }
        else if (op == "LD") {
            if (words.at(1).at(0) == 'R') {
                bitset<2> reg = stoi(words.at(1).substr(1));
                binfile << "110010" << reg.to_string() << "\n";
            }
            else if (words.at(1) == "A") {
                binfile << "11110000";
            }
        }
        else if (op == "PUSH") {
            if (words.at(1) == "A") {
                binfile << "11110011" << "\n";
            }
            else if (words.at(1).at(0) == 'R') {
                bitset<2> reg = stoi(words.at(1).substr(1));
                binfile << "101101" << reg.to_string() << "\n";
            }
            else if (words.at(1) == "AB") {
                lineNumber += 1;
                binfile << "10111111" << "\n";
                binfile << "10111110" << "\n";
            }
            else if (words.at(1) == "AB_bot") {
                binfile << "10111110" << "\n";
            }
            else if (words.at(1) == "RA") {
                lineNumber += 1;
                binfile << "10111101" << "\n";
                binfile << "10111100" << "\n";
            }
            else if (words.at(1) == "FR") {
                binfile << "11110101" << "\n";
            }
            else if (words.at(1) == "ALL") {
                lineNumber += 9;
                binfile << "11110011" << "\n";
                binfile << "10110100" << "\n";
                binfile << "10110101" << "\n";
                binfile << "10110110" << "\n";
                binfile << "10110111" << "\n";
                binfile << "10111111" << "\n";
                binfile << "10111110" << "\n";
                binfile << "10111101" << "\n";
                binfile << "10111100" << "\n";
                binfile << "11110101" << "\n";
            }
        }
        else if (op == "POP") {
            if (words.at(1) == "A") {
                binfile << "11110010" << "\n";
            }
            else if (words.at(1).at(0) == 'R') {
                bitset<2> reg = stoi(words.at(1).substr(1));
                binfile << "101100" << reg.to_string() << "\n";
            }
            else if (words.at(1) == "AB") {
                lineNumber += 1;
                binfile << "10111010" << "\n";
                binfile << "10111011" << "\n";
            }
            else if (words.at(1) == "AB_bot") {
                binfile << "10111010" << "\n";
            }
            else if (words.at(1) == "RA") {
                lineNumber += 1;
                binfile << "10111000" << "\n";
                binfile << "10111001" << "\n";
            }
            else if (words.at(1) == "FR") {
                binfile << "11110100" << "\n";
            }
            else if (words.at(1) == "ALL") {
                lineNumber += 9;
                binfile << "11110100" << "\n";
                binfile << "10111000" << "\n";
                binfile << "10111001" << "\n";
                binfile << "10111010" << "\n";
                binfile << "10111011" << "\n";
                binfile << "10110011" << "\n";
                binfile << "10110010" << "\n";
                binfile << "10110001" << "\n";
                binfile << "10110000" << "\n";
                binfile << "11110010" << "\n";
            }
        }
        else if (op == "SUB") {
            bitset<2> reg = stoi(words.at(1).substr(1));
            binfile << "100001" << reg.to_string() << "\n";
        }
        else if (op == "SLA") {
            if (words.at(1).at(0) == 'R') {
                bitset<2> reg = stoi(words.at(1).substr(1));
                binfile << "100010" << reg.to_string() << "\n";
            }
            else if (isdigit(words.at(1).at(0))) {
                bitset<3> IMM = stoi(words.at(1));
                binfile << "10100" << IMM.to_string() << "\n";
            }
        }
        else if (op == "SRA") {
            if (words.at(1).at(0) == 'R') {
                bitset<2> reg = stoi(words.at(1).substr(1));
                binfile << "100011" << reg.to_string() << "\n";
            }
            else if (isdigit(words.at(1).at(0))) {
                bitset<3> IMM = stoi(words.at(1));
                binfile << "10101" << IMM.to_string() << "\n";
            }
        }
        else if (op == "SRL") {
            bitset<2> reg = stoi(words.at(1).substr(1));
            binfile << "100100" << reg.to_string() << "\n";
        }
        else if (op == "DEC") {
            bitset<2> reg = stoi(words.at(1).substr(1));
            binfile << "100011" << reg.to_string() << "\n";
        }
        else if (op == "NOT") {
            bitset<2> reg = stoi(words.at(1).substr(1));
            binfile << "100100" << reg.to_string() << "\n";
        }
        else if (op == "AND") {
            bitset<2> reg = stoi(words.at(1).substr(1));
            binfile << "100101" << reg.to_string() << "\n";
        }
        else if (op == "OR") {
            bitset<2> reg = stoi(words.at(1).substr(1));
            binfile << "100110" << reg.to_string() << "\n";
        }
        else if (op == "XOR") {
            bitset<2> reg = stoi(words.at(1).substr(1));
            binfile << "100111" << reg.to_string() << "\n";
        }
        else if (op == "NOP") {
            binfile << "11111100" << "\n";
        }
        else if (op == "HLT") {
            binfile << "11111101" << "\n";
        }
        else if (op == "RET") {
            binfile << "11111010" << "\n";
        }
        lineNumber++;
    }
    binfile.close();
}