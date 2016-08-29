#include <iostream>
#include <fstream>
#include <string>
#include <cstring>

using namespace std;

enum Type {
    DIRECTIVE,
    LABEL,
    INSTRUCTION,
    COMMENT,
    UNCONDBRORRET,
    CONDBR,
    NOTYPE,
};

static bool in_comment = false;

inline bool isCommentStarted(const string& s, const int& i) {
    return s[i] == '/' && i+1 < s.size() && s[i+1] == '*';
}

inline bool isCommentEnded(const string& s, const int& i) {
    return s[i] == '*' && i+1 < s.size() && s[i+1] == '/';
}

inline int getEndComment(const string& s, int begin = 0) {
    for (int i = begin; i < s.size(); ++i)
        if (isCommentEnded(s, i))
            return i+2;
    return -1;
}

inline bool isUncondBrOrRet(const string& s, const int& i) {
    return !s.substr(i, 3).compare("jmp") || !s.substr(i, 3).compare("ret");
}

inline bool isCondBr(const string& s, const int& i) {
    if (isUncondBrOrRet(s, i)) return false;
    return s[i] == 'j';
}

char label[64];
int posAfterLabel;

Type getType(const string& s)
{
    int begin = 0;
    if (in_comment) {
        begin = getEndComment(s);
        if (begin == -1) return COMMENT;
        in_comment = false;
        if (begin == s.size()) return COMMENT;
    }

    Type ret = INSTRUCTION;
    int i;
    for (i = begin; i < s.size(); ++i) {
        if (s[i] == ' ' || s[i] == '\t' || s[i] == '\n') continue;
        if (s[i] == '.') { ret = DIRECTIVE; break; }
        if (s[i] == '#') return COMMENT;
        if (isCommentStarted(s, i)) {
            in_comment = true;
            int pos = getEndComment(s, i+2);
            if (begin == -1) return COMMENT;
            in_comment = false;
            if (begin == s.size()) return COMMENT;
            i = pos;
            continue;
        }
        break;
    }

    bool pontentialUncondBrOrRet = false;
    bool pontentialCondBr = false;

    memset(label, 0, 64);
    bool commentAgain = false;
    bool noCommentBegin = false;
    int j = 0;

    while (i < s.size()) {
        if (isCommentStarted(s, i)) {
            in_comment = true;
            int pos = getEndComment(s, i+2);
            if (begin == -1) return COMMENT;
            in_comment = false;
            if (begin == s.size()) return COMMENT;
            i = pos;
            if (noCommentBegin)
                commentAgain = true;
            continue;
        }
        if (ret != DIRECTIVE) {
            pontentialUncondBrOrRet = pontentialUncondBrOrRet || isUncondBrOrRet(s, i);
            pontentialCondBr = pontentialCondBr || isCondBr(s, i);
            if (s[i] == ':') { posAfterLabel = i+1; return LABEL;}

            if (ret != LABEL && !commentAgain) {
                label[j++] = s[i];
            }
        }
        noCommentBegin = true;
        ++i;
    }

    if (!noCommentBegin)
        return NOTYPE;

    if (ret == INSTRUCTION) {
        if (pontentialUncondBrOrRet) return UNCONDBRORRET;
        if (pontentialCondBr) return CONDBR;
    }
    return ret;
}

#include <cstdlib>
inline bool isInteger(const std::string & s)
{
    if(s.empty() || ((!isdigit(s[0])) && (s[0] != '-') && (s[0] != '+'))) return false ;

    char * p ;
    strtol(s.c_str(), &p, 10) ;

    return (*p == '\0') ;
}

int label_n;
void handle(const string& inst,
        bool& existUnhandledInstr,
        bool& existUncondBrOrRet,
        bool& existUnhandledCondBr) {
    Type type = getType(inst);
    switch (type) {
        case CONDBR:
            existUnhandledCondBr = true;
            break;
        case UNCONDBRORRET:
            existUncondBrOrRet = true;
            existUnhandledCondBr = false;
            break;
        case INSTRUCTION:
            if (existUnhandledCondBr) {
                cout << "    jmp BB" << label_n << "\t\t# instrumented" << endl;
                cout << "BB" << label_n << ":" << "\t\t# instrumented" << endl;
                ++label_n;
            }
            existUnhandledInstr = true;
            existUnhandledCondBr = false;
            existUncondBrOrRet = false;
            break;
        case LABEL:
            if (existUnhandledCondBr
                    || (existUnhandledInstr && !existUncondBrOrRet)) {
                cout << "    jmp " << label;
                if (isInteger(label)) cout << 'f';
                cout << "\t\t# instrumented" << endl;
            }
            existUnhandledInstr = false;
            existUncondBrOrRet = false;
            existUnhandledCondBr = false;
            cout << inst.substr(0, posAfterLabel) << endl;
            handle(inst.substr(posAfterLabel, inst.size() - posAfterLabel),
                    existUnhandledInstr,
                    existUncondBrOrRet,
                    existUnhandledCondBr);
            return;
        case NOTYPE:
            return;
        default:
            break;
    }
    cout << inst << endl;
}

int main(int argc, const char *argv[])
{
    if (argc != 2) {
        cout << "Usage: " << argv[0] << " <input asm file>" << endl;
        return 1;
    }

    ifstream fin(argv[1]);
    string inst;

    bool existUnhandledInstr = false;
    bool existUncondBrOrRet = false;
    bool existUnhandledCondBr = false;

    label_n = 0;
    while (getline(fin, inst)) {
        handle(inst,
                existUnhandledInstr,
                existUncondBrOrRet,
                existUnhandledCondBr);
    }
    fin.close();
    return 0;
}
