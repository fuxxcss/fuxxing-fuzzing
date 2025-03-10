
//    Generator 


#pragma once
#include <fstream>
#include <set>
#include <queue>
#include <string>
#include <vector>
#include <antlr4-runtime/ANTLRInputStream.h>
#include "ir.hpp"

#ifdef __PDF__
#include "pdf_lexer.h"
#include "pdf_parser.h"
#include "pdf_listener.h"
#endif

#ifdef __JSON__
#include "json_lexer.h"
#include "json_parser.h"
#include "json_listener.h"
#endif

using std::string;
using std::vector;
using std::set;
using std::queue;
using std::ifstream;
using std::ofstream;

/*  Traits  */
template<typename Trait> struct Gen_Traits;

#ifdef __PDF__
enum class pdf;
template<> struct Gen_Traits<pdf>{
    using Lexer = pdf_lexer;
    using Praser = pdf_parser;
    using Listener = pdf_listener;
};
#endif

#ifdef __JSON__
enum class json;
template<> struct Gen_Traits<json>{
    using Lexer = json_lexer;
    using Praser = json_parser;
    using Listener = json_listener;
};
#endif

template<typename Trait>
class Generator {
    private:

        const string &text;
        string &dict;
    public:
        IR *ir;
        vector<IR *> *ir_library;
        /*  construct   */
        Generator(const string &str,string &path)
            :text(str),dict(path),ir(nullptr),ir_library(nullptr)
            {}
        /*  destruct    */
        ~Generator(){
            delete ir;
            delete ir_library;
        }

        void generate_ir();
        void generate_dict();
};

template<typename Trait>
void Generator<Trait>::generate_ir(){

    antlr4::ANTLRInputStream input(text);
    /*  lexer   */
    Gen_Traits<Trait>::Lexer lexer(&input);
    antlr4::CommonTokenStream tokens(&lexer);
    /*  parser  */
    Gen_Traits<Trait>::Parser parser(&tokens);
    parser.removeErrorListeners();
    /*  listener    */
    Gen_Traits<Trait>::Listener listener;
    antlr4::tree::ParseTreeWalker walker;
    /*  top node    */
    walker.walk(listener,parser.Doc_Format());
    
    if(parser.getNumberOfSyntaxErrors() <= 0){

        ir = listener.ir;
        ir_library = listener.ir_library;
    }
}

/*  todo    */
template<typename Trait>
void Generator<Trait>::generate_dict(){
    
    ifstream dict_in(dict);
    if(!dict_in.is_open()) assert("dict failed.");

    /*  origin,append dict    */
    set<string> origin,append;
    string line;
    while(std::getline(dict_in,line)){
        string tmp = line;
        origin.insert(tmp);
    }
    dict_in.close();

    /*  level order ir */
    queue<IR *> q;
    q.push(ir);
    while(!q.empty()){
        IR *ir = q.pop();
        if(ir->ir_left) q.push(ir->ir_left);
        if(ir->ir_right) q.push(ir->ir_right);
        if(!origin.count(ir->ir_prefix)) append.insert(ir->ir_prefix);
        if(!origin.count(ir->ir_middle)) append.insert(ir->ir_middle);
        if(!origin.count(ir->ir_suffix)) append.insert(ir->ir_suffix);

        switch(ir->ir_type){
        /*  ir vector   */
        case IR_VECTOR:
            size_t size = ir->ir_vector->size();
            while(size){
                q.push(ir->ir_vector[size-1]);
                -- size;
            }
            break;
        #ifdef __PDF__
        /*  pdf keywords    */
        case IR_PDF_OBJ_NAME:
        case IR_PDF_OBJ_STRING:
            if(!origin.count(ir->ir_data.str_data)) append.insert(ir->ir_data.str_data);
            break;
        #endif

        #ifdef __JSON__
        /*  json keywords   */
        /*  ... */
        #endif
        }
    }
    /*  erase empty string  */
    append.erase("");

    ofstream dict_out(dict);
    if(!dict_out.is_open()) assert("dict failed.");
    for(auto i : append) dict_out << i;

}
