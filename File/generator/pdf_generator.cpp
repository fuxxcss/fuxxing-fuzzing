#include <fstream>
#include <set>
#include <queue>
#include "generator.h"
#include "pdf_lexer.h"
#include "pdf_parser.h"
#include "pdf_listener.h"
#include "antlr4-runtime/ANTLRInputStream.h"

using std::set;
using std::queue;
using std::ifstream;
using std::ofstream;

class pdf_generator : public Generator{

    private:
        const string &pdf;
        string &pdf_dict;

    public:
        IR *ir;
        vector<IR *> *ir_library;
        vector<IR *> ir_reuse;
        /*  construct   */
        pdf_generator(const string &str,string &path)
            :pdf(str),pdf_dict(path),ir(nullptr),ir_library(nullptr)
            {}
        /*  destruct    */
        ~pdf_generator(){
            delete ir;
            delete ir_library;
        }

        void generate_ir();
        void generate_dict();
};

void pdf_generator::generate_ir(){

    antlr4::ANTLRInputStream input(pdf);
    pdf_lexer lexer(&input);

    antlr4::CommonTokenStream tokens(&lexer);
    pdf_parser parser(&tokens);

    parser.removeErrorListeners();
    pdf_listener listener;

    antlr4::tree::ParseTreeWalker walker;
    walker.walk(listener,parser.pdf());
    
    if(parser.getNumberOfSyntaxErrors() <= 0){

        ir = listener.ir;
        ir_library = listener.pdf_ir_library;
    }

    free_stack();
}

void pdf_generator::generate_dict(){
    
    ifstream dict_in(pdf_dict);
    if(!dict_in.is_open()) assert("pdf dict failed.");

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
        /*  keyword */
        case IR_PDF_OBJ_NAME:
        case IR_PDF_OBJ_STRING:
            if(!origin.count(ir->ir_data.str_data)) append.insert(ir->ir_data.str_data);
            break;
        }
    }
    /*  erase empty string  */
    append.erase("");

    ofstream dict_out(pdf_dict);
    if(!dict_out.is_open()) assert("pdf dict failed.");
    for(auto i : append) dict_out << i;

}