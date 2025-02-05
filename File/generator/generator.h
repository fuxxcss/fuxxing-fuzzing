
//    Generator 


#pragma once

#include <string>
#include <vector>
using std::string;
using std::vector;

enum IR_TYPE{
    pdf,
    pdf_header,
    pdf_body,
    pdf_obj,
    pdf_obj_name,
    pdf_obj_string,
    pdf_obj_array,
    pdf_obj_dictionary,
    pdf_obj_reference,
    pdf_obj_stream,
    pdf_xref_table,
    pdf_trailer,
    data_int,
    data_str,
    data_real,
    ir_vector,
};

union Data {
    string str_data;
    int int_data;
    float real_data;
    vector *ir_vector;
};

class IR {
    private:
        IR_TYPE ir_type;
        IR *ir_left;
        IR *ir_right;
        string ir_prefix;
        string ir_middle;
        string ir_suffix;
        Data ir_data;
    public:
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

        IR(IR_TYPE type,vector *irv)
            :ir_type(type),
            ir_left(nullptr),
            ir_right(nullptr),
            ir_prefix(""),
            ir_middle(""),
            ir_suffix(""),
            { ir_data.ir_vector = irv; }
        /*  destructor  */
        ~IR(){
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
    case data_int:
        res += to_string(ir_data.int_data);
        break;
    case data_real:
        res += to_string(ir_data.real_data);
        break;
    case data_str:
        res += ir_data.str_data;
        break;
    }

    if(ir_left) res += ir_left->ir_string();

    res += ir_middle;

    if(ir_right) res += ir_right->ir_string();

    res += ir_suffix;

    return res;
}

// Base Class
class Generator {
    public:
        /*  pure virtual funcs  */
        virtual IR *generate_ir() = 0;
        virtual void generate_dict(string dict_path) = 0;
};

