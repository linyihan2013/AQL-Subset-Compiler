#define CREATE 256
#define VIEW 257
#define AS 258
#define EXTRACT 259
#define REGEX 260
#define ON 261
#define FROM 262
#define DOCUMENT 263
#define RETURN 264
#define GROUP 265
#define AND 266
#define OUTPUT 267
#define SELECT 268
#define PATTERN 269
#define TOKEN 270
#define NUM 271
#define ID 272
#define EXPRESSION 273
#define TEXT 274
#include <iostream>
#include <map>
#include <string>
#include <cstdio>
#include <fstream>
#include <vector>
#include <set>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <stack>
#include "unistd.h"
#include "regex.cpp"

using namespace std;

class Token {
    public:

    int col, row, begin, end;
    int tag;
    Token() {
        col = row = 0;
        begin = 0;
        end = 1;
    }
    Token(int t, int c = 0, int r = 0, int b = 0, int e = 1) {
        tag = t;
        col = c;
        row = r;
        begin = b;
        end = e;
    }
    virtual string toString() {
        string s = "";
        s += (char) tag;
        return s;
    }
};

class Num : public Token {
    public:

    int value;
    Num(int v, int c = 0, int r = 0, int b = 0, int e = 1) : Token(v, c, r, b, e) {
        value = v;
    }
    string toString() {
        string s = "";
        s += (char) value;
        return s;
    }
};

class Word : public Token {
    public:

    int index1, index2;
    string lexeme = "";
    Word(string s, int tag, int c = 0, int r = 0, int b = 0, int e = 1) : Token(tag, c, r, b, e) {
        lexeme = s;
        index1 = 0;
        index2 = 1;
    }
    string toString() {
        return lexeme;
    }
};

class Lexer {
    public:

    int line = 1;
    int column = 0;
    char peek = ' ';
    map<string, Word*> words;
    void reserve(Word* w) {
        words[(*w).lexeme] = w;
    }
    Lexer() {
        reserve(new Word("and", AND));
        reserve(new Word("create", CREATE));
        reserve(new Word("view", VIEW));
        reserve(new Word("as", AS));
        reserve(new Word("extract", EXTRACT));

        reserve(new Word("on", ON));
        reserve(new Word("from", FROM));
        reserve(new Word("Document", DOCUMENT));
        reserve(new Word("return", RETURN));
        reserve(new Word("group", GROUP));

        reserve(new Word("regex", REGEX));
        reserve(new Word("output", OUTPUT));
        reserve(new Word("select", SELECT));
        reserve(new Word("pattern", PATTERN));
        reserve(new Word("Token", TOKEN));
    }
    void readch(fstream* stream) {
        (*stream).get(peek);
    }
    bool readch(char c, fstream* stream) {
        readch(stream);
        if (peek != c) return false;
        peek = ' ';
        return true;
    }
    Token* scan(fstream* stream) {
        for (; !(*stream).eof(); readch(stream)) {
            if (peek == ' ' || peek == '\t') {
                column += 1;
                continue;
            } else if (peek == '\n') {
                line += 1;
                column = 0;
            } else break;
        }

        if (isdigit(peek)) {
            int v = 0;
            int col = column;
            do {
                v = 10 * v + peek - '0';
                column += 1;
                readch(stream);
            } while (isdigit(peek) && !stream->eof());
            return new Num(v, col, line);
        }

        if (peek == '/') {
            string b;
            int col = column;

            do {
                while (peek == '\\' && !stream->eof()) readch(stream);
                b += peek;
                column += 1;
                readch(stream);
            } while (peek != '/' && !stream->eof());
            b += peek;
            readch(stream);
            column += 1;

            Word* w = new Word(b, EXPRESSION, col, line);
            return w;
        }

        if (isalpha(peek)) {
            string b;
            int col = column;
            do {
                b += peek;
                column += 1;
                readch(stream);
            } while ((isalpha(peek) || isdigit(peek)) && !stream->eof());
            Word* w = new Word(b, TEXT, col, line);
            if (words.find(b) != words.end()) {
                w = words[b];
            }
            return w;
        }
        Token* tok = new Token(peek, column, line);
        column += 1;
        peek = ' ';
        return tok;
    }
};

class Tokenizer {
    public:
    Word* look;
    int index = 0;
    char peek = ' ';
    string s;
    vector<Word*> tokens;
    int s_index;
    int s_length;

    Tokenizer() {}
    void readch() {
        if (s_index < s_length) {
            peek = s[s_index++];
        }
    }
    void readDocument(fstream* stream) {
        while (!(*stream).eof()) {
            stream->get(peek);
            if (peek == '\n') s += '\r';
            s += peek;
        }
        s.pop_back();

        int in = 0;
        string expression = "[0-9a-zA-Z]+|[!-/:-@[-`{-~]";
        vector<vector<int>> result;
        result = findall(expression.c_str(), s.c_str());
        for (int j = 0; j < result.size(); j++) {
            string s1;
            for (int k = result[j][0]; k < result[j][1]; k++) {
                s1 += s[k];
            }
            for (int k = 0; k < result[0].size(); k += 2) {
                Word* word = new Word(s1, TEXT, 0, 0, result[j][k], result[j][k + 1]);
                word->index1 = in++;
                word->index2 = in;
                tokens.push_back(word);
            }
        }
    }
    string getString() {
        return s;
    }
};