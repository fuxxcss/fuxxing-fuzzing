/*
 *  PDF Parser for Mutation
 */

parser grammar pdf_parser;

options {
    tokenVocab = pdf_lexer;
    superClass = PDFParserBase;
}

@parser::postinclude {
#include <PDFParserBase.h>
}

pdf
    // pdf update
    : pdf_header? pdf_body+
    ;

pdf_body
    : pdf_obj+ pdf_xref_table? pdf_trailer?
    ;

pdf_header
    : data_str SYM_MOD data_str
    ;

pdf_obj
    : pdf_data_array? OBJ pdf_obj+ OBJ_END
    // 5 basic obj types
    | OBJ_NULL
    | OBJ_BOOL
    | (data_int | data_real)
    | pdf_obj_name
    | pdf_obj_string
    // 4 complex obj types
    | pdf_obj_array
    | pdf_obj_dictionary
    | pdf_obj_reference
    | pdf_obj_stream
    ;

pdf_obj_name
    : SYM_FS data_str
    ;

pdf_obj_string
    : SYM_LP data_str SYM_RP
    | SYM_LAB data_str SYM_RAB
    ;

pdf_obj_array
    // all obj in array
    : SYM_LSP pdf_obj* SYM_RSP
    ;

pdf_obj_dictionary
    // key(obj_name) -> value(all obj)
    : SYM_LDAB pdf_obj_dictionary* SYM_RDAB
    | pdf_obj_name pdf_obj
    ;

pdf_obj_reference
    : pdf_data_array? REFERENCE
    ;

pdf_obj_stream
    : pdf_obj_dictionary* STREAM data_str STREAM_END
    ;

pdf_xref_table
    : XREF pdf_xref_table*
    | pdf_data_array? pdf_data_array? (XREF_USE | XREF_FREE)
    ;

pdf_trailer
    : TRAIL pdf_obj_dictionary* TRAIL_XREF data_int TRAIL_END
    ;

pdf_data_array
    : data_int data_int
    | data_str data_str
    ;

data_int
    : DATA_INT
    ;

data_real
    : DATA_REAL
    ;

data_str
    : DATA_STR
    ;