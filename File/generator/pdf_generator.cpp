#include "generator.h"
#include "pdf_lexer.h"
#include "pdf_parser.h"
#include "pdf_listener.h"
#include "antlr4-runtime/ANTLRInputStream.h"

extern 

class pdf_generator : public Generator{
    private:
        string pdf;
    public:
        pdf_generator(string pdf_str):pdf(pdf_str){}
};

IR *pdf_generator::generate_ir(){

    antlr4::ANTLRInputStream input(pdf);
    pdf_lexer lexer(&input);
    antlr4::CommonTokenStream tokens(&lexer);
    pdf_parser parser(&tokens);
    parser.removeErrorListeners();
    pdf_listener listener;
    antlr4::tree::ParseTreeWalker walker;
    walker.walk(listener,parser.pdf());
    
    if(parser.getNumberOfSyntaxErrors() > 0){
        return nullptr;
    }
    else return listener.ir;
}