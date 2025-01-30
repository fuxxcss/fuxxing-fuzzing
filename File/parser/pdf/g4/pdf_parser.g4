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
    : pdf_header pdf_body+
    ;

pdf_body
    : pdf_obj* pdf_xref_table pdf_trailer
    ;

pdf_header
    : DATA_STR SYM_MOD DATA_STR
    ;

pdf_obj
    : DATA_INT DATA_INT OBJ pdf_obj* OBJ_END
    // 5 basic obj types
    | OBJ_NULL
    | OBJ_BOOL
    | (DATA_INT | DATA_REAL)
    | pdf_obj_name
    | pdf_obj_string
    // 4 complex obj types
    | pdf_obj_array
    | pdf_obj_dictionary
    | pdf_obj_reference
    | pdf_obj_stream
    ;

pdf_obj_name
    : SYM_FS DATA_STR
    ;

pdf_obj_string
    : SYM_LP DATA_STR SYM_RP
    | SYM_LAB DATA_STR SYM_RAB
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
    : DATA_INT DATA_INT REFERENCE
    ;

pdf_obj_stream
    : pdf_obj_dictionary* STREAM DATA_STR STREAM_END
    ;

pdf_xref_table
    : XREF pdf_xref_table?
    | DATA_INT DATA_INT DATA_STR DATA_STR (XREF_USE | XREF_FREE)
    ;

pdf_trailer
    : TRAIL pdf_obj_dictionary* TRAIL_XREF DATA_INT TRAIL_END
    ;