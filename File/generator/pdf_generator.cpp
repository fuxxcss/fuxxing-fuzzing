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
        IR *pdf_ir;
        string &pdf_dict;

    public:
        /*  construct   */
        pdf_generator(const string &str,string &path):pdf(str),pdf_dict(path){}
        /*  destruct    */
        ~pdf_generator(){
            delete pdf_ir;
        }

        IR *generate_ir();
        bool generate_dict();
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

        free_stack();
        return nullptr;
    }else{

        pdf_ir = listener.ir;
        return pdf_ir;
    }
}

bool pdf_generator::generate_dict(){
    
    ifstream dict_in(path);
    if(!dict_in.is_open()) return false;

    /*  origin,append dict    */
    set<string> origin,append;
    string line;
    while(std::getline(dict_in,line)){
        string tmp = line;
        origin.insert(tmp);
    }
    dict_in.close();

    /*  level order pdf_ir */
    queue<IR *> q;
    q.push(pdf_ir);
    while(!q.empty()){
        IR *ir = q.pop();
        if(ir->ir_left) q.push(ir->ir_left);
        if(ir->ir_right) q.push(ir->ir_right);
        if(!origin.count(ir->ir_prefix)) append.insert(ir->ir_prefix);
        if(!origin.count(ir->ir_middle)) append.insert(ir->ir_middle);
        if(!origin.count(ir->ir_suffix)) append.insert(ir->ir_suffix);

        switch(ir->ir_type){
        /*  ir vector   */
        case ir_vector:
            size_t size = ir->ir_vector->size();
            while(size){
                q.push(ir->ir_vector[size-1]);
                -- size;
            }
            break;
        /*  keyword */
        case pdf_obj_name:
        case pdf_obj_string:
            if(!origin.count(ir->ir_data.str_data)) append.insert(ir->ir_data.str_data);
            break;
        }
    }
    /*  erase empty string  */
    append.erase("");

    ofstream dict_out(path);
    if(!dict_out.is_open()) return false;
    size_t size = append.size();
    for(int i = 0; i < size; i++) dict_out << append[i];

    return true;
}