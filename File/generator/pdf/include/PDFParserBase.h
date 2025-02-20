
#pragma once

#include "antlr4-runtime.h"

class PDFParserBase : public antlr4::Parser{
    public:
        PDFParserBase & self;
    public:
        PDFParserBase(antlr4::TokenStream *input) : Parser(input),self(*this){}
};