#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "include/caesar_dec.h"
#include "include/caesar_enc.h"
#include "include/subst_dec.h"
#include "include/subst_enc.h"
#include "utils.h"

using namespace std;

// Initialize random number generator in .cpp file for ODR reasons
std::mt19937 Random::rng;

const string ALPHABET = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

// Function declarations go at the top of the file so we can call them
// anywhere in our program, such as in main or in other functions.
// Most other function declarations are in the included header
// files.

// When you add a new helper function, make sure to declare it up here!

/**
 * Print instructions for using the program.
 */
void printMenu();

vector<char> decryptSubstCipher(const QuadgramScorer& scorer, const string& ciphertext);

int main() {
  Random::seed(time(NULL));
  string command;

  cout << "Welcome to Ciphers!" << endl;
  cout << "-------------------" << endl;
  cout << endl;

  vector<string> quadgrams;
  vector<int> counts;
  
  ifstream qin("english_quadgrams.txt");
  string line;
  while (getline(qin, line)) {
    size_t comma = line.find(',');
    if (comma == string::npos) {
      continue;
    }
    string quad = line.substr(0,comma);
    string cnt = line.substr(comma + 1);

    if (!cnt.empty() && cnt.back() == '\r') {
      cnt.pop_back();
    }

    quadgrams.push_back(quad);
    counts.push_back(stoi(cnt));
  }
  QuadgramScorer scorer(quadgrams, counts);


  vector<string> dict;
  string word;
  ifstream fin("dictionary.txt");

  while (getline(fin, word)) {
    for (char& c : word) {
      c = toupper(c);
    }
    dict.push_back(word);
  }

  do {
    printMenu();
    cout << endl << "Enter a command (case does not matter): ";

    // Use getline for all user input to avoid needing to handle
    // input buffer issues relating to using both >> and getline
    getline(cin, command);
    cout << endl;

    if (command == "R" || command == "r") {
      string seed_str;
      cout << "Enter a non-negative integer to seed the random number "
              "generator: ";
      getline(cin, seed_str);
      Random::seed(stoi(seed_str));
    }

    if (command == "C" || command == "c") {
      caesarEncryptCommand();
    }
    if (command == "D" || command == "d") {
      caesarDecryptCommand(dict);
    }
    if (command == "A" || command == "a") {
      applyRandSubstCipherCommand();
    }
    if (command == "E" || command == "e") {
      computeEnglishnessCommand(scorer);
    }
    if (command == "F" || command == "f") {
      string inFile, outFile;
      getline(cin, inFile);
      getline(cin, outFile);

      ifstream finFile(inFile);
      stringstream buffer;
      buffer << finFile.rdbuf();
      string ciphertext = buffer.str();

      vector<char>key = decryptSubstCipher(scorer, ciphertext);

      string plaintext = applySubstCipher(key, ciphertext);

      ofstream foutFile(outFile);
      foutFile << plaintext;
    }
    if (command == "S" || command == "s") {
      decryptSubstCipherCommand(scorer);
    }
    
    cout << endl;

  } while (!(command == "x" || command == "X") && !cin.eof());

  return 0;
}

void printMenu() {
  cout << "Ciphers Menu" << endl;
  cout << "------------" << endl;
  cout << "C - Encrypt with Caesar Cipher" << endl;
  cout << "D - Decrypt Caesar Cipher" << endl;
  cout << "E - Compute English-ness Score" << endl;
  cout << "A - Apply Random Substitution Cipher" << endl;
  cout << "S - Decrypt Substitution Cipher from Console" << endl;
  cout << "F - Decrypt Substitution Cipher from File" << endl;
  cout << "R - Set Random Seed for Testing" << endl;
  cout << "X - Exit Program" << endl;
}

// "#pragma region" and "#pragma endregion" group related functions in this file
// to tell VSCode that these are "foldable". You might have noticed the little
// down arrow next to functions or loops, and that you can click it to collapse
// those bodies. This lets us do the same thing for arbitrary chunks!
#pragma region CaesarEnc

char rot(char c, int amount) {
  int ind = ALPHABET.find(c);

  int newInd = (ind + amount) % 26;

  return ALPHABET[newInd];
}

string rot(const string& line, int amount) {
  string result = "";

  for (char c : line) {

    if (isalpha(c)) {
      char up = toupper(c);
      char rotate = rot(up, amount);
      result += rotate;

    } else if (isspace(c)){
      result += c;
    }
    else {}
  }

  return result;
}

void caesarEncryptCommand() {
  cout << "Enter the text to encrypt:" << endl;
  string plaintext;
  getline(cin, plaintext);

  cout << "Enter the number of characters to rotate by:" << endl;
  string shiftInput;
  getline(cin, shiftInput);

  int shift = stoi(shiftInput);
  string ciphertext = rot(plaintext, shift);

  cout << ciphertext << endl;

}

#pragma endregion CaesarEnc

#pragma region CaesarDec

void rot(vector<string>& strings, int amount) {
  for (string& s : strings) {
    s = rot(s, amount);
  }
}

string clean(const string& s) {
  string out = "";
  for (char c : s) {
    if (isalpha(c)) {
        c = toupper(c);
        out += c;
    }
    else{}
  } 
  return out;
}

vector<string> splitBySpaces(const string& s) {
  vector<string> splitted;
  string curr = "";
  for (size_t i = 0; i < s.size(); i++) {
    if (isspace(s[i])) {
      if (!curr.empty()) {
        splitted.push_back(curr);
        curr = "";
      }
    }
    else {
      curr += s[i];
    }
  }
  if (!curr.empty()) {
    splitted.push_back(curr);
  }
  return splitted;
}

string joinWithSpaces(const vector<string>& words) {
  string out;
  for (size_t i = 0; i < words.size(); i++) {
    if (i > 0) {
      out += ' ';
    }
    out += words[i];
  }
  return out;
}

int numWordsIn(const vector<string>& words, const vector<string>& dict) {
  int goodWords = 0;
  for (string word : words) {
    for (string w : dict) {
        if (word == w) {
          goodWords++;
          break;
        }
      }
  }
  return goodWords;
}

void caesarDecryptCommand(const vector<string>& dict) {
  cout << "Enter the text to decrypt:" << endl;
  string ciphertext;
  if (!getline(cin, ciphertext)) {
    return;
  }

  bool found = false;

  for (int k = 0; k < 26; k++) {
    string line = rot(ciphertext, k);

    vector<string> cleanedWords; 
    vector<string> splitWords = splitBySpaces(line);

    for(const string& word : splitWords) {
      string cw = clean(word);
      if (!cw.empty()) {
        cleanedWords.push_back(cw);
      }
    }

    int totalWords = cleanedWords.size();
    int goodWords = numWordsIn(cleanedWords, dict);

    if ((totalWords > 0) && (goodWords > totalWords / 2)) {
      cout << joinWithSpaces(cleanedWords) << endl;
      found = true;
    }
  }
  if (!found) {
    cout << "No good decryptions found" << endl;
  }
}

#pragma endregion CaesarDec

#pragma region SubstEnc

string applySubstCipher(const vector<char>& cipher, const string& s) {
  string result = "";

  for (char c : s) {
    if (isalpha(c)) {
      char up = toupper(c);
      int index = ALPHABET.find(up);
      result += cipher[index];
    }
    else {
      result += c;
    }
  }

  return result;
}

void applyRandSubstCipherCommand() {
  vector<char> cipher = genRandomSubstCipher();
  string plaintext;

  getline(cin, plaintext);
  string ciphertext = applySubstCipher(cipher, plaintext);
  cout << ciphertext << endl;
}

#pragma endregion SubstEnc

#pragma region SubstDec

double scoreString(const QuadgramScorer& scorer, const string& s) {
  string cs = clean(s);
  double total = 0.0;
  if (cs.length() < 4) {
    return 0.0;
  }
  else {
    for (int i = 0; i <= cs.length() - 4; i++) {
      string quad = cs.substr(i,4);
      double qScore = scorer.getScore(quad);
      total += qScore;
    }
  }
  return total;
}

void computeEnglishnessCommand(const QuadgramScorer& scorer) {
  cout << "Enter a string for scoring:" << endl;
  string line;
  getline(cin, line);

  double score = scoreString(scorer, line);
  
  cout << score << endl;
}

vector<char> hillClimb(const QuadgramScorer& scorer, const string& ciphertext) {
  vector<char> plainToCipher = genRandomSubstCipher();

  vector<char> cipherToPlain(26);
  for (int i = 0; i < 26; i++) {
    int ciph = plainToCipher[i] - 'A';
    cipherToPlain[ciph] = char('A' + i);
  }

  string currentPlain = applySubstCipher(cipherToPlain, ciphertext);
  double currScore = scoreString(scorer, currentPlain);

  vector<char> bestKey = cipherToPlain;
  double bestScore = currScore;

  int fails = 0;
  while (fails < 1000) {
    int i = Random::randInt(25);
    int j = Random::randInt(25);
    while (j==i) {
      j = Random::randInt(25);
    }

    char temporary = cipherToPlain[i];
    cipherToPlain[i] = cipherToPlain[j];
    cipherToPlain[j] = temporary;

    string testPlain = applySubstCipher(cipherToPlain, ciphertext);
    double testScore = scoreString(scorer, testPlain);

    if (testScore > currScore) {
      currScore = testScore;
      currentPlain = testPlain;
      fails = 0;

      if (testScore > bestScore) {
        bestScore = testScore;
        bestKey = cipherToPlain;
      }
    }
    else {
      temporary = cipherToPlain[i];
      cipherToPlain[i] = cipherToPlain[j];
      cipherToPlain[j] = temporary;
      fails++;
    }
  }

  return bestKey;
}

vector<char> decryptSubstCipher(const QuadgramScorer& scorer,
                                const string& ciphertext) {
  vector<char>bestKey;
  double bestScore = -1e9;

  for (int i = 0; i < 25; i++) {
    vector<char>key = hillClimb(scorer, ciphertext);

    string plain = applySubstCipher(key, ciphertext);
    double score = scoreString(scorer, plain);

    if (score > bestScore) {
      bestScore = score;
      bestKey = key;
    }
  }

  return bestKey;
}

void decryptSubstCipherCommand(const QuadgramScorer& scorer) {
  string ciphertext;
  if (!getline(cin, ciphertext)) {
    return;
  }

  vector<char>key = decryptSubstCipher(scorer, ciphertext);
  string plaintext = applySubstCipher(key, ciphertext);

  cout << plaintext << endl;
}

#pragma endregion SubstDec
