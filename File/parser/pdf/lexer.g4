/*
 *  PDF Lexer 
 */

lexer grammar pdf_lexer;

options {
    superClass = PDFLexerBase;
    caseInsensitive = true;
}

@lexer::postinclude {
#include <PDFLexerBase.h>
}

MAGIC       :'%PDF-';
OBJ         :'obj';
ENDOBJ      :'endobj';
SYM_MOD     :'%';
SYM_DOT     :'.';
SYM_SPACE   : [ \t\r\n]+ -> channel(HIDDEN);
DATA_INT    :[0-9]+;
DATA_STR    :

