/*
 *  PDF Parser
 */

parser grammar pdf_parser;

options {
    tokenVocab = pdf_lexer;
    superClass = PDFParserBase;
}

@parser::postinclude {
#include <PDFParserBase.h>
}

pdf_header
    : MAGIC DATA_INT SYM_DOT DATA_INT SYM_MOD DATA_STR
    ;

pdf_obj
    : DATA_INT DATA_INT OBJ pdf_obj_body ENDOBJ
    ;

pdf_obj_body
    :