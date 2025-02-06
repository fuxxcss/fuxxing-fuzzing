#include <stack>
#include <vector>
#include "antlr4-runtime.h"
#include "generator.h"
#include "pdf_listener.h"

using std::stack;
using std::vector;

// stack push middle-process IR
static stack<IR *> *ir_stack;

void free_stack(){

    while(!ir_stack->empty()){
        IR *ir = ir_stack->pop();
        delete ir;
    }

    delete ir_stack;
}

void pdf_listener::enterPdf(pdf_parser::PdfContext *ctx){

    ir_stack = new stack<IR *>;
}

void pdf_listener::exitPdf(pdf_parser::PdfContext *ctx){
    
    size_t body_size = ctx->pdf_body().size();
    vector<IR *> *irv = new vector;
    while(body_size){
        irv->insert(irv->begin(),ir_stack->pop());
        -- body_size;
    }
    IR *right = new IR(ir_vector,irv);
    IR *left = nullptr;
    if(ctx->pdf_header()) left = ir_stack->pop();

    IR *ir = new IR(pdf,left,right);
    pdf_ir = ir;

    delete ir_stack;
}

void pdf_listener::exitPdf_body(pdf_parser::Pdf_bodyContext *ctx){

    IR *right = nullptr;
    if(ctx->pdf_xref_table && ctx->pdf_trailer){
        IR *right_level_2 = ir_stack->pop();
        IR *left_level_2 = ir_stack->pop();

        right = new IR(pdf_body,left_level_2,right_level2);
    }
    else if(ctx->pdf_xref_table || ctx->pdf_trailer){
        right = ir_stack->pop();
    }

    size_t obj_size = ctx->pdf_obj().size();
    vector<IR *> *irv = new vector;
    while(obj_size){
        irv->insert(irv->begin(),ir_stack->pop());
        -- obj_size;
    }

    IR *left = new IR(ir_vector,irv);
    IR *ir = new IR(pdf_body,left,right);

    ir_stack->push(ir);
}

void pdf_listener::exitPdf_header(pdf_parser::Pdf_headerContext *ctx){

    IR *right = ir_stack->pop();
    IR *left = ir_stack->pop();

    string middle = ctx->SYM_MOD()->getText();
    IR *ir = new IR(pdf_header,left,right,"",middle,"");

    ir_stack->push(ir);
}

void pdf_listener::exitPdf_obj(pdf_parser::Pdf_objContext *ctx){

    IR *ir = nullptr;
    if(ctx->OBJ()){
        size_t obj_size = ctx->pdf_obj().size();
        vector<IR *> *irv = new vector;
        while(obj_size){
            irv->insert(irv->begin(),ir_stack->pop());
            -- obj_size;
        }
        IR *right = new IR(ir_vector,irv);
        IR *left = nullptr;
        if(ctx->pdf_data_array()) left = ir_stack->pop();

        string middle = ctx->OBJ()->getText();
        string suffix = ctx->OBJ_END()->getText();

        ir = new IR(pdf_obj,left,right,"",middle,suffix);
    }else{
        IR *left = ir_stack->pop();
        ir = new IR(pdf_obj,left);
    }

    ir_stack->push(ir);
}

void pdf_listener::exitPdf_obj_name(pdf_parser::Pdf_obj_nameContext *ctx){

    IR *left = ir_stack->pop();
    string prefix = ctx->SYM_FS()->getText();

    IR * ir = new IR(pdf_obj_name,left,nullptr,prefix);

    ir_stack->push(ir);
}

void pdf_listener::exitPdf_obj_string(pdf_parser::Pdf_obj_stringContext *ctx){

    string prefix,middle;
    if(ctx->SYM_LP()){
        prefix = ctx->SYM_LP()->getText();
        middle = ctx->SYM_RP()->getText();
    }else{
        prefix = ctx->SYM_LAB()->getText();
        middle = ctx->SYM_RAB()->getText();
    }

    IR *left = ir_stack->pop();
    IR *ir = new IR(pdf_obj_string,left,nullptr,prefix,middle);

    ir_stack->push(ir);
}

void pdf_listener::exitPdf_obj_array(pdf_parser::Pdf_obj_arrayContext *ctx){

    size_t obj_size = ctx->pdf_obj().size();
    vector<IR *> *irv = new vector;
    while(obj_size){
        irv->insert(irv->begin(),ir_stack->pop());
        -- obj_size;
    }
    IR *left = new IR(ir_vector,irv);

    string prefix = ctx->SYM_LSP()->getText();
    string middle = ctx->SYM_RSP()->getText();

    IR *ir = new IR(pdf_obj_array,left,nullptr,prefix,middle);

    ir_stack->push(ir);
}

void pdf_listener::exitPdf_obj_dictionary(pdf_parser::Pdf_obj_dictionaryContext *ctx){

    IR *ir;
    if(ctx->pdf_obj_name()){
        IR *right = ir_stack->pop();
        IR *left = ir_stack->pop();

        ir = new IR(pdf_obj_dictionary,left,right);
    }else{
        size_t obj_dict_size = ctx->pdf_obj_dictionary().size();
        vector<IR *> *irv = new vector;
        while(obj_dict_size){
            irv->insert(irv->begin(),ir_stack->pop());
            -- obj_dict_size;
        }
        IR *left = new IR(ir_vector,irv);

        string prefix = ctx->SYM_LDAB()->getText();
        string middle = ctx->SYM_RDAB()->getText();

        ir = new IR(pdf_obj_dictionary,left,"",prefix,middle);
    }

    ir_stack->push(ir);
}

void pdf_listener::exitPdf_obj_reference(pdf_parser::Pdf_obj_referenceContext *ctx){

    IR *left = nullptr;
    if(ctx->pdf_data_array()) left = ir_stack->pop();

    string middle = ctx->REFERENCE()->getText();

    IR * ir = new IR(pdf_obj_reference,left,nullptr,"",middle);

    ir_stack->push(ir);
}

void pdf_listener::exitPdf_obj_stream(pdf_parser::Pdf_obj_streamContext *ctx){

    IR *right = ir_stack->pop();
    size_t obj_stream_size = ctx->pdf_obj_stream().size();
    vector<IR *> *irv = new vector;
    while(obj_stream_size){
        irv->insert(irv->begin(),ir_stack->pop());
        -- obj_stream_size;
    }
    IR *left = new IR(ir_vector,irv);

    string middle = ctx->STREAM()->getText();
    string suffix = ctx->STREAM_END()->getText();

    IR *ir = new IR(pdf_obj_stream,left,right,"",middle,suffix);

    ir_stack->push(ir);
}

void pdf_listener::exitPdf_xref_table(pdf_parser::Pdf_xref_tableContext *ctx){

    IR *ir;
    if(ctx->XREF()){
        size_t xref_table_size = ctx->pdf_xref_table().size();
        vector<IR *> *irv = new vector;
        while(xref_table_size){
            irv->insert(irv->begin(),ir_stack->pop());
            -- xref_table_size;
        }
        IR *left = new IR(ir_vector,irv);
        string prefix = ctx->XREF()->getText();

        ir = new IR(pdf_xref_table,left,nullptr,prefix);
    }else{
        IR *left = *right = nullptr;

        string suffix = "";
        if(ctx->XREF_USE()) suffix = ctx->XREF_USE()->getText();
        else suffix = ctx->XREF_FREE()->getText();

        size_t data_array_size = ctx->pdf_data_array().size();
        switch(data_array_size){
        case 2:
        right = ir_stack->pop();
        left = ir_stack->pop();
        break;
        case 1:
        left = ir_stack->pop();
        break;
        }

        ir = new(pdf_xref_table,left,right,"","",suffix);
    }

    ir_stack->push(ir);
}

void pdf_listener::exitPdf_trailer(pdf_parser::Pdf_trailerContext *ctx){

    IR *right = ir_stack->pop();
    size_t obj_dict_size = ctx->pdf_obj_dictionary().size();
    vector<IR *> *irv = new vector;
    while(obj_dict_size){
        irv->insert(irv->begin(),ir_stack->pop());
        -- obj_dict_size;
    }
    IR *left = new IR(ir_vector,irv);

    string prefix = ctx->TRAIL()->getText();
    string middle = ctx->TRAIL_XREF()->getText();
    string suffix = ctx->TRAIL_END()->getText();

    IR *ir = new IR(pdf_trailer,left,right,prefix,middle,suffix);

    ir_stack->push(ir);
}

void pdf_listener::exitPdf_data_array(pdf_parser::Pdf_data_arrayContext *ctx){

    IR *right = ir_stack->pop();
    IR *left = ir_stack->pop();
    IR *ir = new IR(pdf_data_array,left,right);

    ir_stack->push(ir);
}

void pdf_listener::exitData_int(pdf_parser::Data_intContext *ctx){

    string text = ctx->DATA_INT()->getText();
    IR *ir = new IR(data_int,stoi(text));

    ir_stack->push(ir);
}

void pdf_listener::exitData_real(pdf_parser::Data_realContext *ctx){

    string text = ctx->DATA_REAL()->getText();
    IR *ir = new IR(data_real,stof(text));

    ir_stack->push(ir);    
}

void pdf_listener::exitData_str(pdf_parser::Data_strContext *ctx){

    string text = ctx->DATA_INT()->getText();
    IR *ir = new IR(data_int,text);

    ir_stack->push(ir);
}