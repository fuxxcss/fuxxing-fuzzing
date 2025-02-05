#include "generator.h"
#include "pdf_lexer.h"
#include "pdf_parser.h"
#include "pdf_listener.h"
#include "antlr4-runtime/ANTLRInputStream.h"

extern 

class pdf_generator : public Generator{
    private:
        string pdf;
        string dict;
    public:
        pdf_generator(string pdf_str,string dict_path):pdf(pdf_str),dict(dict_path){}
        IR *generate_ir();
        void generate_dict();
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

void pdf_generator::generate_dict(){
    
}