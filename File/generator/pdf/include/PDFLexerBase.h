
# pragma onece

#include "antlr4-runtime.h"

class PDFLexerBase : public antlr4::Lexer{
    public:
        PDFLexerBase & self;
    public:
        PDFLexerBase(antlr4::CharStream *input) : Lexer(input),self(*this){}

};