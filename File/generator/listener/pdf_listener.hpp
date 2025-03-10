#include <stack>
#include <vector>
#include "antlr4-runtime.h"
#include "listener.hpp"

using std::stack;
using std::vector;

class  pdf_listener : public listener {

public:

  virtual void enterPdf(pdf_parser::PdfContext *ctx);

  virtual void exitPdf(pdf_parser::PdfContext *ctx);

  virtual void exitPdf_body(pdf_parser::Pdf_bodyContext *ctx);

  virtual void exitPdf_header(pdf_parser::Pdf_headerContext *ctx);

  virtual void exitPdf_obj(pdf_parser::Pdf_objContext *ctx);

  virtual void exitPdf_obj_name(pdf_parser::Pdf_obj_nameContext *ctx);

  virtual void exitPdf_obj_string(pdf_parser::Pdf_obj_stringContext *ctx);

  virtual void exitPdf_obj_array(pdf_parser::Pdf_obj_arrayContext *ctx);

  virtual void exitPdf_obj_dictionary(pdf_parser::Pdf_obj_dictionaryContext *ctx);

  virtual void exitPdf_obj_reference(pdf_parser::Pdf_obj_referenceContext *ctx);

  virtual void exitPdf_obj_stream(pdf_parser::Pdf_obj_streamContext *ctx);

  virtual void exitPdf_xref_table(pdf_parser::Pdf_xref_tableContext *ctx);

  virtual void exitPdf_trailer(pdf_parser::Pdf_trailerContext *ctx);

  virtual void exitPdf_data_array(pdf_parser::Pdf_data_arrayContext *ctx);

  virtual void exitData_int(pdf_parser::Data_intContext *ctx);

  virtual void exitData_real(pdf_parser::Data_realContext *ctx);

  virtual void exitData_str(pdf_parser::Data_strContext *ctx);

};

void pdf_listener::enterPdf(pdf_parser::PdfContext *ctx){

    ir_library = new vector<IR *>;
    ir_stack = new stack<IR *>;
}

void pdf_listener::exitPdf(pdf_parser::PdfContext *ctx){
    
    vector<IR *> *irv = new vector<IR *>;

    for(auto i : ctx->pdf_body()){
        IR *ii = pop();
        irv->insert(irv->begin(),ii);
    }

    IR *right = new IR(IR_VECTOR,irv);
    IR *left = nullptr;
    if(ctx->pdf_header()) {

        left = pop();
    }

    IR *ir = new IR(IR_PDF,left,right);
    pdf_ir = ir;

}

void pdf_listener::exitPdf_body(pdf_parser::Pdf_bodyContext *ctx){

    IR *right = nullptr;
    if(ctx->pdf_xref_table && ctx->pdf_trailer){
        IR *right_level_2 =pop();
        IR *left_level_2 = pop();

        right = new IR(IR_PDF_BODY,left_level_2,right_level2);
    }
    else if(ctx->pdf_xref_table || ctx->pdf_trailer){

        right = pop();
    }

    vector<IR *> *irv = new vector;

    for(auto i : ctx->pdf_obj()){
        IR *ii = pop();
        irv->insert(irv->begin(),ii);
    }

    IR *left = new IR(IR_VECTOR,irv);
    IR *ir = new IR(IR_PDF_BODY,left,right);

    collect(ir);
}

void pdf_listener::exitPdf_header(pdf_parser::Pdf_headerContext *ctx){

    IR *right = pop();
    IR *left = pop();

    string middle = ctx->SYM_MOD()->getText();
    IR *ir = new IR(IR_PDF_HEADER,left,right,"",middle,"");

    collect(ir);
}

void pdf_listener::exitPdf_obj(pdf_parser::Pdf_objContext *ctx){

    IR *ir = nullptr;
    if(ctx->OBJ()){
        vector<IR *> *irv = new vector;

        for(auto i : ctx->pdf_obj()){
            IR *ii = pop();
            irv->insert(irv->begin(),ii);
        }
        
        IR *right = new IR(IR_VECTOR,irv);
        IR *left = nullptr;
        if(ctx->pdf_data_array()) {
            left = pop();
        }

        string middle = ctx->OBJ()->getText();
        string suffix = ctx->OBJ_END()->getText();

        ir = new IR(IR_PDF_OBJ,left,right,"",middle,suffix);
    }else{
        IR *left = pop();
        ir = new IR(IR_PDF_OBJ,left);
    }

    collect(ir);
}

void pdf_listener::exitPdf_obj_name(pdf_parser::Pdf_obj_nameContext *ctx){

    IR *left = pop();
    string prefix = ctx->SYM_FS()->getText();

    IR * ir = new IR(IR_PDF_OBJ_NAME,left,nullptr,prefix);

    collect(ir);
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

    IR *left = pop();
    IR *ir = new IR(IR_PDF_OBJ_STRING,left,nullptr,prefix,middle);

    collect(ir);
}

void pdf_listener::exitPdf_obj_array(pdf_parser::Pdf_obj_arrayContext *ctx){

    vector<IR *> *irv = new vector;

    for(auto i : ctx->pdf_obj()){
        IR *ii = pop();
        irv->insert(irv->begin(),ii);
    }

    IR *left = new IR(IR_VECTOR,irv);

    string prefix = ctx->SYM_LSP()->getText();
    string middle = ctx->SYM_RSP()->getText();

    IR *ir = new IR(IR_PDF_OBJ_ARRAY,left,nullptr,prefix,middle);

    collect(ir);
}

void pdf_listener::exitPdf_obj_dictionary(pdf_parser::Pdf_obj_dictionaryContext *ctx){

    IR *ir;
    if(ctx->pdf_obj_name()){
        IR *right = pop();
        IR *left = pop();

        ir = new IR(IR_PDF_OBJ_DICT,left,right);
    }else{

        vector<IR *> *irv = new vector;

        for(auto i : ctx->pdf_obj_dictionary()){
            IR *ii = pop();
            irv->insert(irv->begin(),ii);
        }
        
        IR *left = new IR(IR_VECTOR,irv);

        string prefix = ctx->SYM_LDAB()->getText();
        string middle = ctx->SYM_RDAB()->getText();

        ir = new IR(IR_PDF_OBJ_DICT,left,"",prefix,middle);
    }

    collect(ir);
}

void pdf_listener::exitPdf_obj_reference(pdf_parser::Pdf_obj_referenceContext *ctx){

    IR *left = nullptr;
    if(ctx->pdf_data_array()) {
        left = pop();
    }

    string middle = ctx->REFERENCE()->getText();

    IR * ir = new IR(IR_PDF_OBJ_REF,left,nullptr,"",middle);

    collect(ir);
}

void pdf_listener::exitPdf_obj_stream(pdf_parser::Pdf_obj_streamContext *ctx){

    IR *right = pop();

    vector<IR *> *irv = new vector;

    for(auto i : ctx->pdf_obj_stream()){
        IR *ii = pop();
        irv->insert(irv->begin(),ii);
    }
    
    IR *left = new IR(IR_VECTOR,irv);

    string middle = ctx->STREAM()->getText();
    string suffix = ctx->STREAM_END()->getText();

    IR *ir = new IR(IR_PDF_OBJ_STREAM,left,right,"",middle,suffix);

    collect(ir);
}

void pdf_listener::exitPdf_xref_table(pdf_parser::Pdf_xref_tableContext *ctx){

    IR *ir;
    if(ctx->XREF()){

        vector<IR *> *irv = new vector;

        for(auto i : ctx->pdf_xref_table()){
            IR *ii = pop();
            irv->insert(irv->begin(),ii);
        }
        
        IR *left = new IR(IR_VECTOR,irv);
        string prefix = ctx->XREF()->getText();

        ir = new IR(IR_PDF_XREF_TABLE,left,nullptr,prefix);
    }else{
        IR *left = *right = nullptr;

        string suffix = "";
        if(ctx->XREF_USE()) suffix = ctx->XREF_USE()->getText();
        else suffix = ctx->XREF_FREE()->getText();

        size_t data_array_size = ctx->pdf_data_array().size();
        switch(data_array_size){
        case 2:
        right = pop();
        left = pop();
        break;
        case 1:
        left = pop();
        break;
        }

        ir = new(pdf_xref_table,left,right,"","",suffix);
    }

    collect(ir);
}

void pdf_listener::exitPdf_trailer(pdf_parser::Pdf_trailerContext *ctx){

    IR *right = pop();

    vector<IR *> *irv = new vector;

    for(auto i : ctx->pdf_obj_dictionary()){
        IR *ii = pop();
        irv->insert(irv->begin(),ii);
    }
   
    IR *left = new IR(IR_VECTOR,irv);

    string prefix = ctx->TRAIL()->getText();
    string middle = ctx->TRAIL_XREF()->getText();
    string suffix = ctx->TRAIL_END()->getText();

    IR *ir = new IR(IR_PDF_TRAILER,left,right,prefix,middle,suffix);

    collect(ir);
}

void pdf_listener::exitPdf_data_array(pdf_parser::Pdf_data_arrayContext *ctx){

    IR *right = pop();
    IR *left = pop();
    IR *ir = new IR(IR_PDF_DATA_ARRAY,left,right);

    collect(ir);
}

void pdf_listener::exitData_int(pdf_parser::Data_intContext *ctx){

    string text = ctx->DATA_INT()->getText();
    IR *ir = new IR(IR_DATA_INT,stoi(text));

    collect(ir);
}

void pdf_listener::exitData_real(pdf_parser::Data_realContext *ctx){

    string text = ctx->DATA_REAL()->getText();
    IR *ir = new IR(IR_DATA_REAL,stof(text));

    collect(ir);    
}

void pdf_listener::exitData_str(pdf_parser::Data_strContext *ctx){

    string text = ctx->DATA_INT()->getText();
    IR *ir = new IR(IR_DATA_STR,text);

    collect(ir);
}
