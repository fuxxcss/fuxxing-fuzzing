
//    Generator 


#pragma once

#include <string>
#include <vector>

using std::string;
using std::vector;

const unsigned int itoa = 4;

enum PDF_TYPE{
    IR_PDF = itoa,
    IR_PDF_HEADER,
    IR_PDF_BODY,
    IR_PDF_OBJ,
    IR_PDF_OBJ_NAME,
    IR_PDF_OBJ_STRING,
    IR_PDF_OBJ_ARRAY,
    IR_PDF_OBJ_DICT,
    IR_PDF_OBJ_REF,
    IR_PDF_STREAM,
    IR_PDF_XREF_TABLE,
    IR_PDF_TRAILER,
    IR_PDF_DATA_ARRAY,
};

enum IR_TYPE{
    IR_DATA_INT = 0,
    IR_DATA_STR,
    IR_DATA_REAL,
    IR_VECTOR,
};

union Data {
    string str_data;
    int int_data;
    float real_data;
};

class IR {

    public:
        IR_TYPE ir_type;
        IR *ir_left;
        IR *ir_right;
        string ir_prefix;
        string ir_middle;
        string ir_suffix;
        vector<IR *> *ir_vector;
        Data ir_data;

        /*  constructor */
        // prefix IR(left) middle IR(right) suffix
        IR(IR_TYPE type,IR *left = nullptr,IR *right = nullptr,string prefix = "",string middle = "",string suffix = "")
            :ir_type(type),
            ir_left(left),
            ir_right(right),
            ir_prefix(prefix),
            ir_middle(middle),
            ir_suffix(suffix),
            {}
        IR(IR_TYPE type,int data)
            :ir_type(type),
            ir_left(nullptr),
            ir_right(nullptr),
            ir_prefix(""),
            ir_middle(""),
            ir_suffix(""),
            { ir_data.int_data = data; }

        IR(IR_TYPE type,string data)
            :ir_type(type),
            ir_left(nullptr),
            ir_right(nullptr),
            ir_prefix(""),
            ir_middle(""),
            ir_suffix(""),
            { ir_data.str_data = data; }

        IR(IR_TYPE type,float data)
            :ir_type(type),
            ir_left(nullptr),
            ir_right(nullptr),
            ir_prefix(""),
            ir_middle(""),
            ir_suffix(""),
            { ir_data.real_data = data; }

        IR(IR_TYPE type,vector<IR *> *irv)
            :ir_type(type),
            ir_left(nullptr),
            ir_right(nullptr),
            ir_prefix(""),
            ir_middle(""),
            ir_suffix(""),
            { ir_vector = irv; }
        /*  deep copy   */
        IR(IR *ir){
            ir_type = ir->ir_type;
            ir_left = new IR(ir->ir_left);
            ir_right = new IR(ir->ir_right);
            ir_prefix = ir->ir_prefix;
            ir_middle = ir->ir_middle;
            ir_suffix = ir->ir_suffix;
            ir_vector = new vector<IR *>(ir->ir_vector);
            ir_data = ir->ir_data;
        }
        /*  destructor  */
        ~IR(){
            if(ir_vector){
                while(!ir_vector->empty()) delete(ir_vector->pop_back());
                delete ir_vector;
            }
            if(ir_left) delete ir_left;
            if(ir_right) delete ir_right;
        }
        /*  ir to string    */
        string ir_string();
};

string IR::ir_string(){

    string res = "";
    res += ir_prefix;

    switch(ir_type){
    case IR_DATA_INT:
        res += to_string(ir_data.int_data);
        break;
    case IR_DATA_REAL:
        res += to_string(ir_data.real_data);
        break;
    case IR_DATA_STR:
        res += ir_data.str_data;
        break;
    case IR_VECTOR:
        size_t size = ir_vector->size();
        for(int i = 0; i < size; i++){
            
            res += ir_vector[i]->ir_string();
        }
    }

    if(ir_left) res += ir_left->ir_string();

    res += ir_middle;

    if(ir_right) res += ir_right->ir_string();

    res += ir_suffix;

    return res;
}

// Base Class
class Generator {
    private:
        const string &str;
        string &dict;
    public:
        IR *ir;
        vector<IR *> ir_library;
        /*  pure virtual funcs  */
        virtual void generate_ir() = 0;
        virtual void generate_dict() = 0;
};

